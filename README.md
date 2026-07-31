# IME Windows Service

Windows named-pipe service for Llavon IME. Platform-neutral model loading,
tokenization, and llama.cpp inference are supplied by the `ime-core` submodule.

The service also owns the interactive per-user process shell:

- `llavon-ime-service.exe` keeps the named-pipe server on an inference worker
  while its main thread owns the notification-area icon.
- `llavon-ime-settings-ui.dll` is loaded on demand from the executable
  directory. It owns a dedicated STA thread, Win32 message loop, settings HWND,
  and inbox `Windows.UI.Xaml` island.
- Settings UI calls are queued to the DLL's STA thread and never execute XAML
  or model work on the inference worker.
- The settings page compares the CI build number embedded at package build time
  with the rolling release's `latest.json` manifest. Commit IDs are diagnostic
  only, so rebases do not affect ordering. Its HTTP request runs on a settings
  worker and is canceled after 300 ms.

The settings module is intentionally a DLL rather than another executable.
Candidate UI will be added as a separate DLL with its own HWND and STA thread.

Source code is divided by runtime responsibility rather than operating-system
name:

- `src/service/`: resident EXE responsibilities, including prediction IPC,
  tray ownership, and loading UI modules.
- `src/settings/`: the settings DLL, its STA runtime, HWND, and XAML island.
- `src/candidate/`: reserved for the future candidate UI DLL; it will not share
  the settings DLL's thread or HWND.

The service keeps the existing executable and IPC compatibility names:

- executable: `llavon-ime-service.exe`
- settings UI module: `llavon-ime-settings-ui.dll`
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
