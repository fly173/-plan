# Windows Build And Packaging

## Development Environment

1. Install Qt 5.15.x or Qt 6.x with either a MinGW or MSVC kit.
2. Open `UCAVDesigner.pro` in Qt Creator, or configure the project with CMake.
3. Select a Release build configuration and run the application.

## CMake Build

```bat
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release
```

If CMake cannot find Qt, provide the Qt kit path:

```bat
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.7.0\mingw_64
```

## qmake Build

```bat
qmake UCAVDesigner.pro
mingw32-make release
```

For MSVC kits, replace `mingw32-make` with the matching Qt kit build tool, such as `nmake`, or build directly in Qt Creator.

## Package The Executable

Enter the Release output directory and run:

```bat
windeployqt UCAVDesigner.exe
```

Copy the deployed output directory to a Windows machine without Qt installed to verify the packaged application.
