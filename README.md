# Balatro 3DS Demake

A C++ demake of Balatro for the Nintendo 3DS, also compatible with PC via SDL2.

## Prerequisites

### PC (macOS/Linux/Windows)
- **CMake** (3.10+)
- **SDL2** (development libraries)

### Nintendo 3DS
- **devkitPro** with **devkitARM**
- **libctru**, **citro3d**, **citro2d** (install via `dkp-pacman`)

## Building and Running

### PC Build
To build and run on your host computer:

```bash
mkdir -p build
cd build
cmake ..
make
./balatro_demake
```

### 3DS Build
To build for the Nintendo 3DS:

```bash
mkdir -p build-3ds
cd build-3ds
cmake -DCMAKE_TOOLCHAIN_FILE=../3ds.toolchain.cmake ..
make
```

This will produce `balatro_demake.3dsx` in the `build-3ds` directory, which can be loaded into an emulator (like Citra) or onto a real 3DS using Homebrew Launcher.

## Project Structure
- `src/`: Source code
- `cmake/`: Custom CMake modules
- `3ds.toolchain.cmake`: Toolchain file for 3DS cross-compilation
- `artifacts/`: Project planning and documentation
