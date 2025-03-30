# Fractal Engine

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

> [!NOTE] The SDL editor and the FTXUI editor are implemented separately and both show different functionality
>
> - GUI editor (SDL2) has the input system implementation
> - FTXUI editor has the physics system implementation
> We plan to integrate them at a later stage but both form part of the unified Fractal architecture.

## Authors

## Third Party Libraries used

We gratefully acknowledge the following libraries and their creators:

- **FTXUI** – Arthur Sonzogni (licensed under the MIT License)
- **PortAudio Portable Real-Time Audio Library** – Ross Bencina and Phil Burk (licensed under the PortAudio license)
- **dr_wav** – David Reid (licensed under the MIT License)
- **SDL** – Sam Lantinga (licensed under the SDL license)
- **Dear ImGui** – Omar Cornut (licensed under the MIT License)

For the full licensing details, please see the [LICENSE.md](./LICENSE.md) file.

## Acknowledgments
