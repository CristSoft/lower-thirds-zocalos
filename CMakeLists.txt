cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version} LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)
if(NOT ENABLE_FRONTEND_API OR NOT ENABLE_QT)
  message(FATAL_ERROR "JCP Sur requires the OBS frontend API and Qt panels.")
endif()

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 6.8 COMPONENTS Widgets Core Svg REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets Qt6::Svg)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-main.cpp src/model.cpp src/renderer.cpp src/panels.cpp src/resources.qrc)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(MSVC)
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /utf-8)
endif()

option(JCP_BUILD_TESTS "Build offscreen integration checks" OFF)
if(JCP_BUILD_TESTS)
  enable_testing()
  add_executable(jcp-check tests/check.cpp src/model.cpp src/renderer.cpp src/panels.cpp src/resources.qrc)
  set_target_properties(jcp-check PROPERTIES AUTORCC ON)
  target_include_directories(jcp-check PRIVATE src)
  target_link_libraries(jcp-check PRIVATE Qt6::Core Qt6::Widgets Qt6::Svg)
  # Uses the real libobs scene graph with a small in-process frontend harness.
  # Run manually with an OBS runtime directory on PATH; no user instance is touched.
  add_executable(jcp-obs-check tests/obs-check.cpp src/model.cpp src/renderer.cpp src/panels.cpp src/resources.qrc)
  set_target_properties(jcp-obs-check PROPERTIES AUTORCC ON)
  target_include_directories(jcp-obs-check PRIVATE src)
  target_link_libraries(jcp-obs-check PRIVATE Qt6::Core Qt6::Widgets Qt6::Svg OBS::libobs OBS::obs-frontend-api)
  if(MSVC)
    target_compile_options(jcp-obs-check PRIVATE /utf-8)
  endif()
  add_test(NAME jcp-check COMMAND jcp-check)
  if(WIN32)
    set_tests_properties(jcp-check PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows")
  else()
    set_tests_properties(jcp-check PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
endif()
