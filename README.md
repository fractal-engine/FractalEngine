<p align="center">
  <img src="https://i.imgur.com/5Ssv6YY.png" alt="Fractal Logo"/>
</p>

<p align="center">
  <!-- <a href="https://github.com/fractal-engine/Fractal_3D/releases">Releases</a> | -->
  <!-- <a href="#screenshots">Screenshots</a> | -->
  <!-- <a href="#features">Features</a> | -->
  <!-- <a href="https://github.com/fractal-engine/Fractal_3D/wiki">Wiki</a> | -->
  <a href="#goals">Goals</a> |
  <a href="#project-status">Project Status</a> |
  <a href="#getting-started">Getting Started</a> |
  <a href="https://github.com/fractal-engine/Fractal_3D/blob/main/CONTRIBUTING.md">Contributing</a> |
  <a href="https://github.com/fractal-engine/Fractal_3D/blob/main/LICENSE.md">License</a> |
  <a href="https://github.com/fractal-engine/Fractal_3D/wiki">Documentation</a>
<br/>
<br/>
<a href="https://github.com/fractal-engine/Fractal_3D/releases">
  <img alt="platforms" src="https://img.shields.io/badge/platforms-Windows%20%7C%20macOS%20%7C%20Linux-blue?style=flat-square"/>
</a>
<a href="https://github.com/fractal-engine/Fractal_3D">
  <img alt="top language" src="https://img.shields.io/github/languages/top/fractal-engine/Fractal_3D?color=%230xfffff&style=flat-square"/>
</a>
<!-- <a href="https://github.com/fractal-engine/Fractal_3D">
  <img alt="size" src="https://img.shields.io/github/repo-size/fractal-engine/Fractal_3D?style=flat-square"/>
</a> -->
<br/>
<a href="https://github.com/fractal-engine/Fractal_3D/issues">
  <img alt="issues" src="https://img.shields.io/github/issues-raw/fractal-engine/Fractal_3D.svg?color=yellow&style=flat-square"/>
</a>
<a href="https://github.com/fractal-engine/Fractal_3D/pulls">
  <img alt="pulls" src="https://img.shields.io/github/issues-pr-raw/fractal-engine/Fractal_3D?color=yellow&style=flat-square"/>
</a>
<br/>
<a href="https://github.com/fractal-engine/Fractal_3D/blob/main/LICENSE.md">
  <img alt="license" src="https://img.shields.io/github/license/fractal-engine/Fractal_3D?color=green&style=flat-square"/>
</a>
<a href="https://discord.gg/ScXrJnbuJU">
  <img alt="discord" src="https://img.shields.io/discord/1331206947495477328?label=discord&logo=discord&logoColor=white&color=7389D8&style=flat-square"/>
</a>
</p>

## Contributing

Any contributions should follow the guidelines laid out in the [CONTRIBUTING.md](./CONTRIBUTING.md) file.

Any bugs or suggestions should be reported as [Issues](https://github.com/fractal-engine/Fractal_3D/issues).

## Getting Started

Follow these steps to set up and run the project:

1. **Build the Project:**  
   Open your terminal and run:

   ```bash
   xmake build
   ```

2. **Run the Project:**  
   After a successful build, start the project with:

   ```bash
   xmake run
   ```

3. **(Optional) Generate Compile Commands:**  
   If your development tools require a `compile_commands.json` file, generate it by running:

   ```bash
   xmake project -k compile_commands
   ```

4. **(Optional) Switch to Debug Mode:**  
   To enable debugging features, switch to debug mode:

   ```bash
   xmake f -m debug
   ```

5. **To build with Visual Studio Debugger**

    ```bash
    xmake project -k vsxmake -m "debug"
    ```

## Goals

## Project Status

## Thirdparty Libraries

### Building BGFX

This engine uses a [CMake-based fork of bgfx](https://github.com/bkaradzic/bgfx.cmake) for easier integration. Follow these steps to clone and build the library:

> **Note**: This port is made to be used as a C++ library only. Some features such as bindings or dynamic tools might not work. For full flexibility, use the original bgfx repo with [GENie](https://github.com/bkaradzic/GENie).

#### Cloning and Building

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

#### Enabling Shaderc (BGFX Shader Compiler)

To compile shaders used by the engine, you need to build `shaderc.exe`, BGFX's cross-platform shader compiler.

> **Warning**: shaderc must be built manually before compiling any `.sc` shader files. This only needs to be done once unless you clean your build or update BGFX.

#### Build Shaderc on Windows (VS2022 example)

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

> Tip: This path is already hardcoded in the engine’s `xmake.lua`, so once `shaderc` is built, shader compilation will work automatically when building the project.

## Important Note

The SDL editor and the FTXUI editor are implemented separately and both show different functionality

- SDL editor has the input system implementation
- FTXUI editor has the physics system implementation

We plan to integrate them at a later stage but both form part of the unified Fractal architecture.

## Authors

## Third Party Libraries used

We gratefully acknowledge the following libraries and their creators:

- **FTXUI** – Arthur Sonzogni (licensed under the MIT License)
- **PortAudio Portable Real-Time Audio Library** – Ross Bencina and Phil Burk (licensed under the PortAudio license)
- **dr_wav** – David Reid (licensed under the MIT License)
- **SDL** – Sam Lantinga (licensed under the SDL license)
- **Dear ImGui** – Omar Cornut (licensed under the MIT License)
- **bgfx** – Branimir Karadzic (licensed under the zlib license)

For the full licensing details, please see the [LICENSE.md](./LICENSE.md) file.
