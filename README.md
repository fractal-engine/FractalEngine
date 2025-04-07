# Readme and some notes here

## Build the project

Use the following to build the project:
```bash
xmake build
```
To run the project, use:
```bash
xmake run fractal
```

You may need the following to generate compile_command:.
```bash
xmake project -k compile_commands
```

Use the command below to change to debug mode:
```bash
xmake f -m debug
```

## Building BGFX

This engine uses a [CMake-based fork of bgfx](https://github.com/bkaradzic/bgfx.cmake) for easier integration. Follow these steps to clone and build the library:

> **Note**: This port is made to be used as a C++ library only. Some features such as bindings or dynamic tools might not work. For full flexibility, use the original bgfx repo with [GENie](https://github.com/bkaradzic/GENie).

### Cloning and Building

```bash
# Clone the bgfx.cmake fork
git clone https://github.com/bkaradzic/bgfx.cmake.git src/thirdparty/bgfx.cmake
cd src/thirdparty/bgfx.cmake

# Pull in all bgfx dependencies
git submodule update --init --recursive

# Configure and build the library
cmake -S. -Bcmake-build
cmake --build cmake-build
```

---

## Enabling Shaderc (BGFX Shader Compiler)

To compile shaders used by the engine, you need to build `shaderc.exe`, BGFX's cross-platform shader compiler.

> **Warning**: shaderc must be built manually before compiling any `.sc` shader files. This only needs to be done once unless you clean your build or update BGFX.

### Build Shaderc on Windows (VS2022 example)

```bash
cd src/thirdparty/bgfx.cmake

# Make sure all submodules are pulled in (important!)
git submodule update --init --recursive

# Configure build (use your preferred toolchain if not VS2022)
cmake -B .build/win64_vs2022 -DCMAKE_BUILD_TYPE=Release -DBGFX_BUILD_TOOLS=ON

# Build just the shaderc tool
cmake --build .build/win64_vs2022 --config Release --target shaderc
```

After this, `shaderc.exe` will be available at:
```
src/thirdparty/bgfx.cmake/.build/win64_vs2022/bin/shaderc.exe
```

Your build system (e.g., XMake) should point to this executable when compiling shaders.

>  Tip: This path is already hardcoded in the engine’s `xmake.lua`, so once `shaderc` is built, shader compilation will work automatically when building the project.


## Important Note

The SDL editor and the FTXUI editor are implemented separately and both show different functionality
- SDL editor has the input system implementation
- FTXUI editor has the physics system implementation

We plan to integrate them at a later stage but both form part of the unified Fractal architecture.


## Third Party Libraries used

We wish to thank and acknowledge the following libraries and their creators:

- FTXUI - Copyright (c) 2019 Arthur Sonzogni under MIT license
- PortAudio Portable Real-Time Audio Library - Copyright (c) 1999-2011 Ross Bencina and Phil Burk
- dr_wav - Copyright 2020 David Reid under MIT license

Full licensing statement available at LICENSE.md




