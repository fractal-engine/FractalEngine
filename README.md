# Readme and some notes here

## Build the project

Use 
```bash
xmake build
```
to build the project, and
```bash
xmake run
```
to run the project.

You may need
```bash
xmake project -k compile_commands
```
To generate compile_commands.

Use
```bash
xmake f -m debug
```
To change to debug mode.

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




