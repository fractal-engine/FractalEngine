#include "editor/editor_layer.h"
#include "components/camera_controls.h"
#include "components/game_canvas.h"
#include "components/log_panel.h"
#include "components/toolbar.h"
#include "core/engine_globals.h"
#include "core/logger.h"
#include "core/view_ids.h"
#include "game/game_test.h"
#include "platform/platform_utils.h"
#include "subsystem/subsystem_manager.h"
#include "subsystem/window_manager.h"
#include "tools/imgui_utils.h"

#include <backends/imgui_impl_sdl2.h>
#include "editor/vendor/imgui_impl_bgfx.h"

#include <SDL.h>
#include <chrono>
#include <thread>

EditorLayer::EditorLayer(std::unique_ptr<RendererBase>& renderer)
    : renderer_(renderer) {}

EditorLayer::~EditorLayer() {}

void EditorLayer::Run() {
  Logger::getInstance().Log(LogLevel::Info, "EditorLayer main loop start");

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_Implbgfx_Init(ViewID::UI);

  InitImGui();
  ImGui_ImplSDL2_InitForOther(WindowManager::GetWindow());

  SDL_Event event;
  while (!quit_) {
    // 0. SDL events
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);

      if (event.type == SDL_KEYDOWN) {
        Key key = sdl_key_to_key(event);
        if (key != Key::NONE)
          HandleInput(key);
      }

      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        WindowManager::OnWindowResize(event.window.data1, event.window.data2);
        ImGui::GetIO().DisplaySize =
            ImVec2((float)event.window.data1, (float)event.window.data2);
      }

      if (event.type == SDL_QUIT) {
        quit_ = true;
        editor_exit_pressed();
      }
    }

    /* 1 - Clear background first */
    bgfx::setViewClear(ViewID::UI_BACKGROUND, BGFX_CLEAR_COLOR, 0x1e1e1eff,
                       1.0f, 0);
    bgfx::touch(ViewID::UI_BACKGROUND);

    /* 2 - start ImGui frame so we get actual canvasViewportW/H */
    BeginImGuiFrame(WindowManager::GetWindow());
    RenderUI();

    /* 3 - Configure/prepare scene framebuffer with updated size */
    auto* graphics = static_cast<GraphicsRenderer*>(renderer_.get());
    graphics->PrepareFrame();

    /* 4 - Let game render into correctly‐sized FBO */
    if (is_game_started_) {
      SubsystemManager::GetGameManager()
          ->Render();  // Render sky->SCENE, terrain->SCENE_N(0)
    }

    /* 5 - Submit ImGui to UI view */
    ImGui::Render();
    ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());

    /* 6 - Present frame */
    renderer_->Render();
    bgfx::frame();
  }
  Logger::getInstance().Log(LogLevel::Info, "EditorLayer loop exited");
}

void EditorLayer::RequestUpdate() {}

void EditorLayer::Shutdown() {
  Logger::getInstance().Log(LogLevel::Info, "Shutting down EditorLayer");
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void EditorLayer::HandleInput(Key key) {
  if (ImGui::GetIO().WantCaptureKeyboard && !game_canvas_hovered_)
    return;

  switch (key) {
    case Key::DIGIT_0:
      quit_ = true;
      editor_exit_pressed();
      return;
    case Key::DIGIT_1:
      if (!is_game_started_) {
        is_game_started_ = true;
        game_start_pressed();
      }
      return;
    case Key::DIGIT_2:
      if (is_game_started_) {
        is_game_started_ = false;
        game_end_pressed();
      }
      return;
    default:
      break;
  }

  if (!is_game_started_)
    return;

  InputEvent input_event(key);
  if (auto* gm = SubsystemManager::GetGameManager().get()) {
    input_event.pressed_frame_ = gm->GetFrameCount();
  }
  SubsystemManager::GetInput()->FowardInputEvent(input_event,
                                                 input_event.pressed_frame_);
}

void EditorLayer::RenderUI() {
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(
      ImVec2(WindowManager::GetWidth(), WindowManager::GetHeight()));
  ImGui::Begin("Fractal Engine", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

  Components::ToolbarCallbacks cb{.onStart =
                                      [&]() {
                                        if (!is_game_started_) {
                                          is_game_started_ = true;
                                          game_start_pressed();
                                        }
                                      },
                                  .onStop =
                                      [&]() {
                                        if (is_game_started_) {
                                          is_game_started_ = false;
                                          game_end_pressed();
                                        }
                                      },
                                  .onAddObject =
                                      []() {
                                        // TODO: add game object
                                      },
                                  .onRemoveObject =
                                      []() {
                                        // TODO: remove game object
                                      },
                                  .onQuit =
                                      [&]() {
                                        quit_ = true;
                                        editor_exit_pressed();
                                      }};

  Components::Toolbar(cb);
  Components::GameCanvas(is_game_started_, game_canvas_hovered_);
  Components::LogPanel();
  Components::CameraControls();

  ImGui::End();
}

void EditorLayer::BeginImGuiFrame(SDL_Window* window) {
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2((float)WindowManager::GetWidth(),
                          (float)WindowManager::GetHeight());
  io.DisplayFramebufferScale =
      ImVec2(WindowManager::GetDPIScale(), WindowManager::GetDPIScale());

  ImGui_ImplSDL2_NewFrame();  // platform backend
  ImGui_Implbgfx_NewFrame();  // renderer backend
  ImGui::NewFrame();          // ImGui begins
}
