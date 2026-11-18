"""Fetch official, pinned OBS dependencies with resumable, verified downloads.

Build-time utility only. OBS never executes this file.
"""
from concurrent.futures import ThreadPoolExecutor
from hashlib import sha256
from pathlib import Path
from urllib.error import URLError
from urllib.request import Request, urlopen
import json
import sys

ROOT = Path(__file__).resolve().parent.parent
DEST = ROOT / ".deps"


def digest(path):
    h = sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def fetch(item):
    name, spec = item
    version = spec["version"]
    filename = f"{version}.zip" if name == "obs-studio" else (
        f"windows-deps-{'qt6-' if name == 'qt6' else ''}{version}-x64"
        + (f"-v{spec['revision']['windows-x64']}" if spec.get("revision", {}).get("windows-x64") else "") + ".zip"
    )
    url = f"{spec['baseUrl']}/{filename}" if name == "obs-studio" else f"{spec['baseUrl']}/{version}/{filename}"
    target = DEST / filename
    expected = spec["hashes"]["windows-x64"]
    partial = target.with_suffix(".zip.part")
    if target.exists():
        if digest(target) == expected:
            print(f"{name}: SHA-256 correcto", flush=True)
            return
        if not partial.exists():
            target.replace(partial)
        else:
            target.unlink()
    last_error = None
    for attempt in range(1, 4):
        offset = partial.stat().st_size if partial.exists() else 0
        try:
            headers = {"Range": f"bytes={offset}-"} if offset else {}
            with urlopen(Request(url, headers=headers), timeout=20) as response:
                append = offset > 0 and response.status == 206
                if append and not response.headers.get("Content-Range", "").startswith(f"bytes {offset}-"):
                    raise ValueError("El servidor respondió con un rango inesperado.")
                with partial.open("ab" if append else "wb") as output:
                    for chunk in iter(lambda: response.read(256 * 1024), b""):
                        output.write(chunk)
            if digest(partial) != expected:
                partial.unlink()
                raise ValueError("El SHA-256 descargado no coincide con buildspec.json.")
            partial.replace(target)
            print(f"{name}: descargado y verificado", flush=True)
            return
        except (OSError, URLError, ValueError) as error:
            last_error = error
            print(f"{name}: intento {attempt}/3: {error}", file=sys.stderr, flush=True)
    raise RuntimeError(f"No se pudo descargar {name}. Se conserva el parcial para reintentar. {last_error}")


def main():
    DEST.mkdir(exist_ok=True)
    specs = json.loads((ROOT / "buildspec.json").read_text(encoding="utf-8"))["dependencies"]
    with ThreadPoolExecutor(max_workers=3) as pool:
        list(pool.map(fetch, specs.items()))


if __name__ == "__main__":
    main()
