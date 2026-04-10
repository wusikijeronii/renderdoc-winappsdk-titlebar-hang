Minimal Win32 test app for reproducing Windows App SDK window decoration behavior.

## Requirements

Before building, make sure these tools are installed and available in `PATH`:

- CMake 3.21+
- A C++ compiler/toolchain for Windows
- Windows SDK
- `nuget.exe`
- `cppwinrt.exe`

Notes:

- `nuget.exe` is used to download the required Windows App SDK packages during CMake configure.
- `cppwinrt.exe` is used to generate C++/WinRT headers from the downloaded `.winmd` metadata.
- Internet access is required on the first configure if the packages are not already present in `build/packages`.

## Build

From the repository root:

```powershell
cmake -S . -B build
cmake --build build
```

## Run

After a successful build, run:

```powershell
.\build\decor_test.exe
```

The build also copies the required bootstrap DLL next to the executable automatically.

## Output

Expected files after build:

- `build/decor_test.exe`
- `build/Microsoft.WindowsAppRuntime.Bootstrap.dll`

## Clean rebuild

If you want to force package/header regeneration:

```powershell
Remove-Item -Recurse -Force .\build
cmake -S . -B build
cmake --build build
```
