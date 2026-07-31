# IME Windows Service

Windows named-pipe service for Llavon IME. Platform-neutral model loading,
tokenization, and llama.cpp inference are supplied by the `ime-core` submodule.

The service keeps the existing executable and IPC compatibility names:

- executable: `llavon-ime-service.exe`
- named pipe: `\\.\pipe\llavon-ime`
- model path: `LLAVON_IME_MODEL_PATH`
- tables path: `LLAVON_IME_TABLES_DIR`

## Build

Initialize the nested `ime-core` submodule, then pass a vcpkg toolchain:

```powershell
git submodule update --init --recursive
cmake --preset windows -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build --preset windows
cmake --install build/windows --config Release
```

The top-level CMake project configures `ime-core` and the Windows service separately.
Each project uses its own `vcpkg.json` and its own `vcpkg_installed` tree.
