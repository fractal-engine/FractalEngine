# `/platform` Directory Overview

The files inside `/platform` are responsible for handling platform-specific abstractions such as HiDPI scaling, native window bindings, and rendering surface setup.

---

## Files

### ✅ `platform_utils.h`

**Purpose:**  
Declares a **cross-platform interface** for interacting with platform-specific logic like window size, DPI scaling, and BGFX platform initialization.

**Functions:**

- `GetDrawableSize(SDL_Window* window, int* out_w, int* out_h)`  
  - Returns the actual framebuffer size in physical pixels (e.g., Retina-aware).
- `float GetDPIScale(SDL_Window* window)`  
  - Computes the scale factor between logical and drawable resolution.
- `void* CreateMetalLayer(void* native_window)`  
  - On macOS, attaches a `CAMetalLayer` to a Cocoa `NSWindow`.  
  - On Windows/Linux, returns `nullptr`.
- `SetupBGFXPlatformData(bgfx::Init& init, SDL_Window* window)`  
  - Populates `bgfx::Init` with the correct `platformData` fields depending on the OS and windowing system.
- `InitSDLForImGui(SDL_Window* window)`  
  - Calls the appropriate `ImGui_ImplSDL2_*` initializer depending on platform (Metal vs. OpenGL/DirectX/etc.).

---

### ✅ `platform_utils.cpp`

**Platform(s):** Windows + Linux  
**Purpose:**  
Implements all functions from `platform_utils.h` for Windows and Linux targets.

**Highlights:**

- Uses `SDL_GetRendererOutputSize` and `SDL_GL_GetDrawableSize` for window metrics.
- Extracts native handles for DirectX or X11 via `SDL_SysWMinfo`.
- Uses `ImGui_ImplSDL2_InitForOther()` as the default ImGui backend initializer.

---

### ✅ `platform_utils.mm`

**Platform:** macOS  
**Purpose:**  
Implements the same interface as `platform_utils.cpp`, but using Cocoa APIs to interact with `NSWindow`, `CAMetalLayer`, and Metal.

**Highlights:**

- Attaches `CAMetalLayer` to `NSView` to prepare rendering surface.
- Retrieves HiDPI scale factor from `backingScaleFactor`.
- Sets `bgfx::Init`'s Metal layer handle (`nwh`) using native Cocoa pointers.
- Calls `ImGui_ImplSDL2_InitForMetal()` to initialize ImGui on macOS.

---

## Deprecated / Replaced Files

These older files were merged into `platform_utils.*`:

| Old File | Covered By |
|----------|----------------|
| `window_macos.h/.mm` | `platform_utils.h/mm` (`GetDrawableSize`, `GetDPIScale`, `CreateMetalLayer`) |
| `bgfx_macos.h/.mm`   | `platform_utils.h/mm` (`SetupBGFXPlatformData`) |
| `imgui_macos.h/.mm`  | `platform_utils.h/mm` (`InitSDLForImGui`) |
