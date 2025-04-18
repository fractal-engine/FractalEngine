# `/platform` Directory Overview

This document describes the purpose and use of each file in the `/platform` directory, which centralizes all macOS‑specific bridging logic.

---

## `window_macos.h` / `window_macos.mm`

**Purpose:**  
Low‑level bridge between SDL and Cocoa/Metal for windowing and HiDPI support.

**Key Functions (extern "C"):**

- `WindowManager_CreateMetalLayer(void* cocoaWindow)`  
  Creates and attaches a `CAMetalLayer` to an `NSWindow` obtained from SDL.
- `WindowManager_GetDPIScale(SDL_Window* sdlWindow)`  
  Computes the Retina scale factor by comparing SDL logical size to Metal drawable size.
- `WindowManager_GetDrawableSize(SDL_Window* window, int* out_w, int* out_h)`  
  Fetches the actual pixel dimensions of the Metal backing layer.

**Usage:**  
Called by higher‑level platform modules (`bgfx_macos`) to get DPI info and metal layer pointers with Objective‑C.

---

## `bgfx_macos.h` / `bgfx_macos.mm`

**Purpose:**  
High‑level BGFX integration on macOS.

**Key Function:**

- `namespace bgfx_macos { void SetupPlatformData(bgfx::Init& init, SDL_Window* window); }`  
  1. Queries drawable size via `WindowManager_GetDrawableSize`  
  2. Creates/attaches `CAMetalLayer` via `WindowManager_CreateMetalLayer`  
  3. Populates `init.resolution` and `init.platformData` for BGFX

---

## `imgui_macos.h` / `imgui_macos.mm`

**Purpose:**  
Platform‑specific setup for Dear ImGui’s SDL backend on Metal.

**Key Function:**

- `namespace imgui_macos { void InitSDLForMetal(SDL_Window* window); }`
Wraps `ImGui_ImplSDL2_InitForMetal(window)` so that core ImGui‑renderer code avoids `#ifdef __APPLE__`.
