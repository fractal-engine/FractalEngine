#include "editor/editor_layer.h"
#include "components/camera_controls.h"
#include "components/console_panel.h"
#include "components/game_canvas.h"
#include "components/hierarchy_panel.h"
#include "components/inspector_panel.h"
#include "components/menu_bar.h"
#include "components/status_bar.h"
#include "components/toolbar.h"
#include "core/engine_globals.h"
#include "core/logger.h"
#include "core/view_ids.h"
#include "editor/resource/theme/dark_theme.hpp"
#include "game/game_test.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "platform/platform_utils.h"
#include "subsystem/subsystem_manager.h"
#include "subsystem/window_manager.h"
#include "tools/imgui_utils.h"

#include <backends/imgui_impl_sdl2.h>
#include "editor/vendor/imgui/imgui_impl_bgfx.h"

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
  Theme::Initialize();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable Multi-Viewport

  io.ConfigViewportsNoAutoMerge = true;
  io.ConfigViewportsNoTaskBarIcon = true;

  // handle docking behavior
  // ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;

  // backends
  ImGui_Implbgfx_Init(ViewID::UI);
  ApplyImGuiShaders();
  ImGui_ImplSDL2_InitForOther(WindowManager::GetWindow());

  io.DisplaySize = ImVec2((float)WindowManager::GetWidth(),
                          (float)WindowManager::GetHeight());
  io.DisplayFramebufferScale =
      ImVec2(WindowManager::GetDPIScale(), WindowManager::GetDPIScale());
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
        built_layout_ = false;
      }

      if (event.type == SDL_QUIT) {
        quit_ = true;
        editor_exit_pressed();
      }
    }

    /* 1 - Clear background */
    bgfx::setViewClear(ViewID::UI_BACKGROUND, BGFX_CLEAR_COLOR, 0x1e1e1eff,
                       1.0f, 0);
    bgfx::touch(ViewID::UI_BACKGROUND);  // process background

    /* 2 - Configure and prepare scene framebuffer */
    auto* graphics = static_cast<GraphicsRenderer*>(renderer_.get());
    graphics->PrepareFrame();  // set up scene framebuffer view

    /* 3 - Let game render into scene framebuffer */
    if (is_game_started_) {
      SubsystemManager::GetGameManager()->Render();  // Render to ViewID::SCENE
    }

    /* 4 - start ImGui frame, samples from scene texture */
    BeginImGuiFrame(WindowManager::GetWindow());
    RenderUI();

    /* 5 - Submit ImGui to UI view */
    ImGui::Render();
    ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());

    // 6 - Update platform windows
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      bgfx::frame(false);
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
    }

    /* 7 - Present frame */
    renderer_->Render();
    bgfx::frame();
  }
  Logger::getInstance().Log(LogLevel::Info, "EditorLayer loop exited");
}

void EditorLayer::RequestUpdate() {}

void EditorLayer::Shutdown() {
  Logger::getInstance().Log(LogLevel::Info, "Shutting down EditorLayer");
  ImGui_Implbgfx_Shutdown();
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
  static constexpr const char* root_dock =
      "##MainDockHost";  // Set root dock ID

  ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);
  ImGui::SetNextWindowViewport(vp->ID);

  host_flags_ = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoDocking |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoFocusOnAppearing;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});

  // ———— Dockspace —————
  ImGui::Begin(root_dock, nullptr, host_flags_);
  dock_flags_ = ImGuiDockNodeFlags_PassthruCentralNode;

  dock_id_ = ImGui::GetID(root_dock);
  ImGui::DockSpace(dock_id_, ImVec2(0, 0), dock_flags_);
  ImGui::End();

  // ———— Menu bar —————
  Components::MenuBar(quit_, debug_highlight_ids_, debug_show_metrics_,
                      debug_show_log_, debug_activate_picker_);

  // ———— Status bar —————
  Components::StatusBar();
  ImGui::PopStyleVar(3);
}

void EditorLayer::RenderUI() {
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;

  DockSpace();

  // ——— Build layout ———
  if (!built_layout_) {

    ImGui::DockBuilderRemoveNode(dock_id_);
    ImGui::DockBuilderAddNode(dock_id_, ImGuiDockNodeFlags_None);
    ImGui::DockBuilderSetNodeSize(dock_id_, ImGui::GetIO().DisplaySize);

    // splits:
    ImGuiID left = ImGui::DockBuilderSplitNode(dock_id_, ImGuiDir_Left, 0.20f,
                                               nullptr, &dock_id_);
    ImGuiID right = ImGui::DockBuilderSplitNode(dock_id_, ImGuiDir_Right, 0.20f,
                                                nullptr, &dock_id_);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(dock_id_, ImGuiDir_Down, 0.25f,
                                                 nullptr, &dock_id_);
    ImGuiID top = ImGui::DockBuilderSplitNode(dock_id_, ImGuiDir_Up, 0.15f,
                                              nullptr, &dock_id_);

    // docked panels
    ImGui::DockBuilderDockWindow("Toolbar", top);
    ImGui::DockBuilderDockWindow("Hierarchy", left);
    ImGui::DockBuilderDockWindow("Inspector", right);
    ImGui::DockBuilderDockWindow("Scene", dock_id_);
    ImGui::DockBuilderDockWindow("Console", bottom);
    ImGui::DockBuilderDockWindow("Camera", right);

    ImGui::DockBuilderFinish(dock_id_);
    built_layout_ = true;
  }

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
  ImGui::Begin("Toolbar", nullptr);
  Components::Toolbar(cb);
  ImGui::End();

  // -------- LEFT : hierarchy + assets -------------------------------------
  static std::vector<std::string> demo_names = {"Camera", "Terrain", "Sun",
                                                "Player"};
  ImGui::Begin("Hierarchy", nullptr);
  Components::HierarchyPanel(demo_names, "assets");
  ImGui::End();

  // -------- MIDDLE : game view --------------------------------------------
  ImGui::Begin("Scene", nullptr);
  Components::GameCanvas(is_game_started_, game_canvas_hovered_);
  ImGui::End();

  // -------- RIGHT : inspector ---------------------------------------------
  static std::vector<Components::Transform> demo_transform(demo_names.size());
  ImGui::Begin("Inspector", nullptr);
  Components::Inspector(demo_transform);
  ImGui::End();

  //--------------------------- PANELS ------------------------------
  ImGui::Begin("Console", nullptr);
  Components::ConsolePanel();
  ImGui::End();

  // Camera Controls
  ImGui::Begin("Camera", nullptr);
  Components::CameraControls();
  ImGui::End();

  //------------------------- IMGUI DEBUG ---------------------------
  if (debug_show_metrics_) {
    ImGui::SetNextWindowDockID(0, ImGuiCond_Always);
    ImGui::ShowMetricsWindow(&debug_show_metrics_);
  }
  if (debug_show_log_) {
    ImGui::SetNextWindowDockID(0, ImGuiCond_Always);
    ImGui::ShowDebugLogWindow(&debug_show_log_);
  }
  if (debug_activate_picker_) {
    ImGui::DebugStartItemPicker();
    debug_activate_picker_ = false;  // reset
  }
}

void EditorLayer::BeginImGuiFrame(SDL_Window* window) {
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