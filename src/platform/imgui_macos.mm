#import <backends/imgui_impl_sdl2.h>
#import "imgui_macos.h"

namespace imgui_macos {

void InitSDLForMetal(SDL_Window* window) {
    ImGui_ImplSDL2_InitForMetal(window);
}

} // namespace imgui_macos
