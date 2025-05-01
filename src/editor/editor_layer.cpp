#include "editor/editor_layer.h"
#include "components/camera_controls.h"
#include "components/console_panel.h"
#include "components/game_canvas.h"
#include "components/hierarchy_panel.h"
#include "components/inspector_panel.h"
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

EditorLayer* EditorLayer::s_instance_ = nullptr;

EditorLayer::EditorLayer(std::unique_ptr<RendererBase>& renderer)
    : renderer_(renderer) {
  s_instance_ = this;
}

EditorLayer::~EditorLayer() {}

// static accessor:
EditorLayer* EditorLayer::Get() {
  return s_instance_;
}

void EditorLayer::Initialize() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto& io = ImGui::GetIO();

  window_flags_ = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                 ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

  ImGui::StyleColorsDark();

  // backends
  ImGui_Implbgfx_Init(ViewID::UI);
  InitImGui();  // TODO: change naming and move logic
  ImGui_ImplSDL2_InitForOther(WindowManager::GetWindow());
}

void EditorLayer::Run() {
  Logger::getInstance().Log(LogLevel::Info, "EditorLayer main loop start");

  Initialize();

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
    bgfx::touch(ViewID::UI_BACKGROUND);  // Ensure background view is processed

    /* 2 - Configure and prepare scene framebuffer */
    auto* graphics = static_cast<GraphicsRenderer*>(renderer_.get());
    graphics->PrepareFrame();  // This sets up the scene framebuffer view

    /* 3 - Let game render into scene framebuffer */
    if (is_game_started_) {
      SubsystemManager::GetGameManager()->Render();  // Renders to ViewID::SCENE
    }

    /* 4 - start ImGui frame, samples from scene texture */
    BeginImGuiFrame(WindowManager::GetWindow());
    RenderUI();  // includes GameCanvas which samples scene texture

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

void EditorLayer::DockSpace() {
  ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowViewport(vp->ID);
  ImGui::SetNextWindowPos(vp->Pos);
  ImGui::SetNextWindowSize(vp->Size);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  ImGui::Begin("Dockspace", nullptr, window_flags_);
  ImGuiID dockID = ImGui::GetID("Dockspace");
  ImGui::DockSpace(dockID, ImVec2(0, 0),
                   ImGuiDockNodeFlags_PassthruCentralNode);
  ImGui::End();

  ImGui::PopStyleVar(3);
}

void EditorLayer::RenderUI() {
  // full window
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

  DockSpace();

  //--------------------------- TOP TOOLBAR ----------------------------------
  Components::ToolbarCallbacks cb{.onStart =
                                      [&] {
                                        if (!is_game_started_) {
                                          is_game_started_ = true;
                                          game_start_pressed();
                                        }
                                      },
                                  .onStop =
                                      [&] {
                                        if (is_game_started_) {
                                          is_game_started_ = false;
                                          game_end_pressed();
                                        }
                                      },
                                  .onQuit =
                                      [&] {
                                        quit_ = true;
                                        editor_exit_pressed();
                                      }};
  Components::Toolbar(cb);

  //--------------------------- LAYOUT ---------------------------------
  const ImVec2 avail = ImGui::GetContentRegionAvail();
  const float totalW = avail.x;
  const float totalH = avail.y;
  const float leftW = 240.0f;
  const float rightW = 300.0f;
  const float bottomH = 150.0f;
  const float centerW = totalW - leftW - rightW;
  const float centerH = totalH - bottomH;

  // -------- LEFT : hierarchy + assets -------------------------------------
  static std::vector<std::string> demoNames = {"Camera", "Terrain", "Sun",
                                               "Player"};
  Components::HierarchyPanel(demoNames, "assets");
  ImGui::SameLine();

  // -------- MIDDLE : game view --------------------------------------------
  Components::GameCanvas(is_game_started_, game_canvas_hovered_);
  ImGui::SameLine();

  // -------- RIGHT : inspector ---------------------------------------------
  static std::vector<Components::Transform> demoTf(demoNames.size());
  Components::Inspector(demoTf);

  //--------------------------- PANELS ------------------------------
  Components::ConsolePanel();
  ImGui::SameLine();

  // Camera Controls
  Components::CameraControls();
}

void EditorLayer::BeginImGuiFrame(SDL_Window* window) {
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Viewports
  io.DisplaySize = ImVec2((float)WindowManager::GetWidth(),
                          (float)WindowManager::GetHeight());
  io.DisplayFramebufferScale =
      ImVec2(WindowManager::GetDPIScale(), WindowManager::GetDPIScale());

  ImGui_ImplSDL2_NewFrame();  // platform backend
  ImGui_Implbgfx_NewFrame();  // renderer backend
  ImGui::NewFrame();          // ImGui begins
}

// ── Selection API ─────────────────────────────────────────────────────────
void EditorLayer::SetSelectedEntity(int id) {
  selected_entity_ = id;
  last_selected_entity_ = selected_entity_;
}

int EditorLayer::GetSelectedEntity() const {
  return selected_entity_;
}

int EditorLayer::GetLastSelectedEntity() const {
  return last_selected_entity_;
}