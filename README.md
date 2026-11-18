# Zócalos para OBS — 1.9.2

Plugin nativo en C++17 y Qt 6, con una fuente transparente, un panel acoplable en **Paneles** de OBS y una ventana independiente de **Diseño**. Funciona sin Python, Lua, navegador, servidor local ni obs-websocket.

[Descargas y versiones](https://github.com/CristSoft/lower-thirds-zocalos/releases) · [Herramientas Gráficas](https://herramientasgraficas.web.app/)

## Funciones

- **Zócalos:** biblioteca sin límite fijo de cantidad, nombre interno, hasta tres líneas y botón de ícono **▶ Lanzar al vivo** a la izquierda del nombre. Las tarjetas son acordeones: por defecto solo se ve el nombre y el botón; al pulsar la flecha se abre el editor y se cierra el anterior. Agregar, duplicar, buscar, limpiar la búsqueda con ✕, eliminar, importar y exportar. Arrastrar el nombre permite cambiar el orden con una línea de inserción y desplazamiento automático en los extremos; el orden se guarda en la biblioteca. Con un filtro activo se reordenan solamente las filas visibles y se conservan las posiciones de las ocultas.
- Barra compacta con íconos para ocultar, agregar, exportar, importar y abrir Diseño mediante un engranaje. Cada botón tiene descripción al pasar el mouse y nombre accesible. Las filas evitan los márgenes de título del tema de OBS.
- **Diseño de zócalos:** ventana independiente, sin acoplamiento, que se abre con el engranaje. La vista previa y sus controles permanecen fijos arriba mientras se desplazan los ajustes. Los selectores 1, 2 y 3 permiten combinar las líneas del ejemplo; los botones ▶, ■ y loop reproducen una vez, detienen o repiten la animación. Ofrece catorce variantes, cuatro colores editables como HEX con una muestra clickeable a la derecha, tipografía, selector de logos incluidos e importación PNG/JPG/WebP/SVG, tamaño del emblema (32–196 px), ocultar ícono, redondeo, entrada/salida, ancho fijo y permanencia automática. Emblema, redondeo, entrada/salida y ancho fijo tienen deslizador y campo numérico sincronizados.
- Estilos disponibles: **Barra y emblema**, **Clásico**, **Banda**, **Estela dorada · círculo y revelado**, **Línea superior**, **Línea inferior**, **Doble línea**, **Tarjeta lateral** y **Marco fino**. Los diseños nuevos respetan ancho fijo, colores, tipografía, logo propio, visibilidad y tamaño del ícono. Estela dorada se inspira en la [variante del editor web](https://herramientasgraficas.web.app/): círculo con doble aro, nota musical predeterminada, fondo degradado, filetes y revelado suave siguiendo al círculo. Permite reemplazar la nota por un logo propio; restaurar el ícono usa el predeterminado del diseño seleccionado. Se reimplementa en Qt, sin navegador ni reproducción del JavaScript del sitio.
- **Barridos verticales:** la línea superior sube desde abajo y revela el texto debajo de ella; la inferior baja desde arriba y revela el texto por encima. En Doble línea, ambas salen del centro hacia arriba y abajo y revelan el espacio entre ellas. La salida invierte el mismo movimiento y respeta la duración configurada.
- La primera variante se basa en el lower third del proyecto: placa oscura, acento ámbar, barra viajera, ícono genérico y unión líquida. El movimiento se reimplementa con QPainter; no ejecuta el CSS original ni promete equivalencia cuadro por cuadro.
- Entrada y salida automática inversas. Al lanzar otro zócalo, sale el anterior antes de entrar el siguiente. El botón de la fila activa (y de la pendiente, durante un cambio) pasa a **■ STOP rojo**. Pulsarlo reproduce la animación de salida y cancela la cola. El **STOP general**, fijo en la barra, se pone rojo únicamente mientras hay una placa al aire y vuelve al color normal al terminar la salida. Retira también las muestras de Vista previa con animación, aunque la fila activa esté filtrada o plegada.
- Las ediciones se aplican al siguiente lanzamiento. El contenido en escena conserva sus textos y diseño.
- Las líneas vacías desaparecen. La placa conserva siempre el **Ancho fijo** configurado, tanto con textos cortos como largos. El texto reduce su tamaño únicamente cuando no entra en el espacio útil de la placa. Con redondeo mayor de 40 px aumenta el margen derecho de los textos; al ocultar el símbolo también se reserva espacio a la izquierda y la barra vertical termina al comienzo de la curva. El valor está expresado en píxeles del lienzo de OBS y no puede superar su ancho. Se respeta la escala de la fuente en la escena; si hace falta, se corrige su posición horizontal para que no sobresalga por la derecha.
- Guardado automático con escritura atómica y respaldo de bibliotecas dañadas.

## Cambios de la versión 1.9.2

Franjas + logo, Recipiente llenado y Moderno usan las siluetas, paletas y secuencias de entrada del editor web. El texto mantiene sus proporciones y se reduce según el espacio disponible al cambiar el ancho fijo.

El plugin incluye los diseños de la web: Barra y emblema (Original), Estela dorada, Franjas diagonales, Franjas + logo, Recipiente llenado, Moderno y Cinta 3D. Cinta 3D usa los fotogramas de Blender incluidos en el editor web y conserva el ancho fijo configurado en OBS. El movimiento se dibuja de forma nativa y se ajusta a la duración de entrada y salida del plugin.

El **Isotipo de la iglesia** se dibuja desde el SVG oficial y toma el ajuste **Acento e ícono**. Franjas + logo y Moderno arrancan con el emblema claro de la web cuando usan su símbolo predeterminado, y lo recolorean al personalizar el acento.

En **Diseño > Logo** podés usar el predeterminado del estilo, ocultarlo con **Ninguno** o elegir **Isotipo de la iglesia**, **Símbolo circular**, **Música · Corchea doble**, **Corona y Biblia** o **Biblia**. Usá **Importar logo…** para seleccionar una imagen propia. El último logo importado queda en el selector aunque elijas otro y se recupera al volver a abrir OBS. Al actualizar se conservan las bibliotecas y los logos personalizados existentes.

## Instalación en Windows x64

Requiere OBS Studio de 64 bits con Qt 6.8; se compila con SDK de OBS 31.1.1 y Qt 6.8.3. OBS proporciona las DLL de Qt.

1. Cerrá OBS y ejecutá `Zocalos-OBS-v1.9.2-Instalador-Windows-x64.exe`.
2. Aceptá la solicitud de permisos de administrador y seguí el asistente.
3. Abrí OBS y activá **Paneles > Zócalos**. El engranaje abre **Diseño de zócalos**. Las ubicaciones guardadas se conservan al actualizar.

También podés usar `Zocalos-OBS-Windows-x64.zip`: descomprimilo y copiá la carpeta `jcp-sur-lower-thirds` completa a `%ProgramData%\obs-studio\plugins\`.

Por compatibilidad con versiones anteriores, los nombres internos de la carpeta, la DLL, la fuente y la configuración mantienen su identificador histórico. Las escenas y bibliotecas existentes se conservan.

El paquete incluye `install-windows.ps1` como alternativa para copiar los archivos. Es únicamente un instalador: OBS carga la DLL nativa. Debe conservarse junto a la carpeta `jcp-sur-lower-thirds` extraída del ZIP. No mover esa carpeta antes de ejecutar el instalador. Ahora también reconoce la DLL compilada al ejecutarlo desde este proyecto, aunque se haya movido la carpeta de `release` durante una instalación manual. El archivo usa UTF-8 con BOM para mostrar bien las tildes en Windows PowerShell 5.1. `./install-windows.ps1 -ValidateOnly` muestra las rutas que utilizaría sin copiar archivos. Si el destino requiere privilegios, ejecutar desde PowerShell como administrador.

Para OBS portable, copiar la DLL a `<OBS>\obs-plugins\64bit\` y el contenido de `data` a `<OBS>\data\obs-plugins\jcp-sur-lower-thirds\`. Alternativamente: `install-windows.ps1 -ObsRoot 'D:\OBS-portable'`.

Para acoplar el panel de Zócalos abajo, hacer clic derecho sobre el fondo del panel y elegir **Acoplar abajo**. El menú contextual también permite elegir arriba, izquierda o derecha. La ventana de Diseño no se acopla. El panel admite una franja de poca altura y barras de desplazamiento para seguir editando dentro de ese espacio.

## Instalador ejecutable para Windows

El proyecto incluye una definición de instalador de Inno Setup en `installer/zocalos.iss`. Solicita permisos de administrador, instala el plugin en la carpeta compartida de OBS y comprueba que OBS esté cerrado. Mantiene la biblioteca de zócalos y la configuración existentes. Esta ruta es para la instalación normal compartida de OBS; para una instalación portable se conserva el procedimiento manual anterior.

Para generarlo, primero compilar el plugin y preparar la carpeta `release`; luego ejecutar `installer/build-installer.ps1` con Inno Setup instalado. También se puede indicar la ruta al compilador con `-Iscc`. El desinstalador elimina los archivos del plugin, pero deja intacta la biblioteca del usuario.

## Ubuntu 26.04 x64

La compilación para Linux está dirigida a Ubuntu 26.04 de 64 bits con OBS Studio 32.1 o posterior instalado mediante APT. Consultá [Releases](https://github.com/CristSoft/lower-thirds-zocalos/releases) para ver qué paquetes se publicaron. El paquete `.deb` instala el plugin en las rutas del OBS de APT. Flatpak y Snap tienen sus propios entornos y rutas.

Si está disponible el paquete, cerrá OBS e instalalo con `sudo apt install ./Zocalos-OBS-v1.9.2-Ubuntu-26.04-x64.deb`. Al abrir OBS, activá **Paneles > Zócalos**.

La automatización de GitHub compila con `cmake --preset ubuntu-x86_64 -DJCP_BUILD_TESTS=ON`, ejecuta las pruebas Qt y comprueba la instalación y el enlace del módulo antes de generar el paquete de Ubuntu.

## Operación

1. Pulsar el engranaje para abrir **Diseño** y revisar la vista previa. Escribir colores como `#FFFFFF` (también acepta el valor sin `#`) o pulsar la muestra de color a la derecha para elegirlos. El tamaño del emblema cambia su altura y ancho dentro del espacio reservado; se mantiene centrado. Los cambios se aplican al próximo lanzamiento.
2. En **Zócalos**, desplegar el nombre para editar una, dos o tres líneas. Al agregar o duplicar una tarjeta, se abre su editor automáticamente. Al plegarla queda visible el botón ▶ a la izquierda, que lanza el zócalo sin desplegarlo. Para cambiar el orden, arrastrar el nombre hasta la línea de inserción entre tarjetas. Un clic normal en el nombre sigue abriendo el editor. El botón ✕ junto al buscador borra el filtro y devuelve el foco al campo.
3. Pulsar **Lanzar al vivo**. El plugin crea o reutiliza la fuente **Zócalos**, la agrega a la escena de programa actual, la hace visible y la pone encima de las otras fuentes.
4. Pulsar el **STOP rojo** de la fila o **Ocultar zócalo** en la barra para reproducir la animación de salida y retirarlo. Con permanencia `0 s` se mantiene hasta esa acción; con otro valor, el tiempo empieza al terminar la entrada.

En Diseño, los selectores de líneas y la reproducción son exclusivos de la vista previa: no cambian los textos guardados ni la salida de OBS. El ejemplo usa los textos del zócalo seleccionado y completa las líneas vacías con texto de muestra. La reproducción incluye entrada, permanencia y salida; si la permanencia general es 0, el ejemplo usa 2 segundos para poder completar el ciclo. **Detener** vuelve a mostrar el gráfico completo y cerrar Diseño detiene la reproducción.

En modo estudio, el lanzamiento actúa sobre **Programa**. La primera vez, la fuente se ubica abajo con márgenes proporcionales al lienzo. Se puede mover y escalar desde OBS; los próximos lanzamientos conservan esa transformación. La altura nativa es de 196 px; el ancho nativo se calcula a partir del ancho fijo configurado y la escala de la fuente en la escena, sin depender del texto. La ubicación inicial se adapta a lienzos horizontales y verticales. El ancho fijo de fábrica es 1440 px del lienzo; en un lienzo menor se limita a ese ancho.

La fuente aparece solamente en escenas a las que se haya agregado. Para usarla en varias, agregar la fuente existente en cada escena o lanzar una vez desde cada escena. Cambiar de escena no relanza la animación. Si se oculta un grupo o escena que contiene el zócalo, revisar también la composición de Programa.

La biblioteca es global a esta instalación de OBS, compartida entre perfiles y colecciones. Cambiar de colección solicita ocultar el gráfico. Al reiniciar OBS comienza oculto aunque la fuente siga guardada en una escena.

El **ojito de cada tarjeta** muestra sus textos reales en la escena seleccionada en **Vista previa de OBS**, con el diseño actual; no abre la ventana de Diseño ni completa líneas vacías con textos de ejemplo. Activa Modo estudio cuando hace falta. Cada muestra usa una fuente independiente para evitar cambios en un zócalo que ya esté en Programa. Si la escena seleccionada también está al aire (incluidas escenas anidadas), crea y selecciona una copia de sus elementos llamada `… · Vista previa zócalos …`; las cámaras y medios siguen siendo las mismas fuentes. La escena original continúa intacta en Programa. Las copias quedan en la colección para que el operador pueda conservarlas o eliminarlas. Al lanzar al vivo desde una escena que contenía una muestra, se oculta esa muestra para evitar dos zócalos superpuestos. El STOP general también retira con animación cualquier muestra que el operador haya pasado a Programa con una transición.

Los valores predeterminados de bibliotecas nuevas son: diseño **Barra y emblema**, fondo `#17120B`, acento e ícono `#F7A813`, primera línea `#F5F6FA`, líneas 2 y 3 `#F7A813`, **Advent Sans**, ícono visible, emblema **170 px**, redondeo **90 px**, entrada/salida **2200 ms** y permanencia **15 s**. Actualizar el plugin conserva los valores explícitos de la biblioteca existente.

**Advent Sans está incluida** tanto dentro de la DLL como en `data/fonts/AdventSans-Logo.otf`. Qt la carga privadamente al abrir el plugin: no requiere instalar fuentes en Windows ni permisos de administrador para registrarla. Se distribuye sin modificar, con su licencia SIL OFL 1.1 y la referencia a la [distribución oficial](https://www.adventist.design/global-elements/advent-sans/) en `data/fonts/`.

## Guardado e intercambio

OBS asigna al plugin una carpeta de configuración con `library.json`; normalmente es `%AppData%\obs-studio\plugin_config\jcp-sur-lower-thirds\library.json`. Los íconos propios se copian a `icons` dentro de esa carpeta.

**Exportar biblioteca** guarda textos y configuración en JSON. **Importar** agrega los textos con identificadores nuevos; preserva el diseño actual y las entradas existentes. Para trasladar diseño e íconos, cerrar OBS y copiar también la carpeta de configuración completa. Si cambia la ubicación, seleccionar el ícono nuevamente. El JSON no incorpora los bytes de imágenes.

## Compilación

Requisitos: Visual Studio 2022 con C++ x64, Windows SDK 10.0.22621 o posterior, CMake 3.28+ y conexión a GitHub. Dependencias oficiales con hashes SHA-256 fijados en `buildspec.json`. Basado en [obsproject/obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate). Los paneles utilizan [obs_frontend_add_dock_by_id](https://docs.obsproject.com/reference-frontend-api#c.obs_frontend_add_dock_by_id).

```powershell
cmake --preset windows-x64 -DJCP_BUILD_TESTS=ON
cmake --build build_x64 --config Release --parallel
# Agregar .deps/obs-deps-qt6-2025-07-11-x64/bin a PATH para ejecutar tests.
ctest --test-dir build_x64 -C Release --output-on-failure
cmake --install build_x64 --config Release --prefix release
```

`build-windows.ps1` compila, verifica y empaqueta. No instala el plugin en OBS automáticamente.

Los tests verifican persistencia, validación JSON, 1501 entradas, entrada/cambio/salida, permanencia, transparencia, catorce diseños, acordeón exclusivo, lanzamiento desde la cabecera plegada, actualización del nombre y acoplamiento real con QMainWindow. El instalador se verifica también en Windows PowerShell 5.1 con una instalación temporal, actualización y rutas con espacios y corchetes. `jcp-check.exe <carpeta>` exporta capturas y fotogramas para revisión visual.

Verificación 1.7.0: compilación Release con MSVC; pruebas Qt de persistencia, render, valores predeterminados, registro de fuente incluida, ancho fijo con textos cortos y largos, márgenes para esquinas redondeadas, barra contenida sin símbolo, deslizadores y entrada manual, salida animada al pulsar STOP, color rojo condicionado a una placa al aire, estados de botones, selección de catorce estilos, limpieza de búsqueda y eventos Qt de arrastre y suelta. También se verifican la dirección de cada barrido vertical, las barras móviles y la revelación del texto entre ellas. Se comprueban el orden guardado, el movimiento al principio/final, el filtro con filas ocultas, el indicador, el desplazamiento automático y la conservación del contenido al aire. `jcp-obs-check.exe` usa el grafo real de escenas y fuentes de libobs con un frontend de prueba en memoria: verifica destino de Vista previa, aislamiento cuando coincide con Programa, fuentes independientes, reemplazo de muestras y STOP general. Con `jcp-obs-check.exe <carpeta-OBS>` verifica también el backend D3D11 en un destino fuera de pantalla y que la textura GPU respete el ancho fijo, lo conserve al acortar los textos y solo cambie al modificar ese ajuste. Se ejecuta con el directorio `bin/64bit` de ese runtime de OBS en `PATH`; no abre ni modifica la instalación o las colecciones del usuario. Las capturas de `verification` provienen de las pruebas Qt. La instalación del usuario permanece intacta durante la compilación y verificación.

## Alcance

Un zócalo de operación al aire por vez; las muestras de Vista previa son instantáneas independientes que el operador puede pasar a Programa mediante una transición de OBS. No incluye atajos globales, cola automática, control remoto, íconos animados ni configuración independiente por fuente. Las variantes comparten las tres líneas y los ajustes generales; cambian composición y entrada/salida. Barra y emblema conserva la unión líquida y Estela dorada tiene el recorrido circular y el revelado degradado. El plugin agrega gráficos a la composición; no inicia transmisiones ni grabaciones.

Código GPL-2.0-or-later: ver `LICENSE`. Los recursos gráficos conservan la titularidad y condiciones de sus respectivos propietarios.
