#include "editor/editor_layer.h"
#include "editor/resources/theme/dark_theme.hpp"
#include "editor/runtime/application.h"
#include "engine/core/engine_globals.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "gui/asset_browser.h"
#include "gui/camera_controls.h"
#include "gui/console_panel.h"
#include "gui/file_explorer.h"
#include "gui/game_canvas.h"
#include "gui/hierarchy_panel.h"
#include "gui/inspector_panel.h"
#include "gui/menu_bar.h"
#include "gui/status_bar.h"
#include "gui/toolbar.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_utils.h"
#include "platform/platform_utils.h"
#include "platform/window_manager.h"
#include "resources/decorators/drop_shadows.h"

#include <backends/imgui_impl_sdl2.h>
#include "editor/vendor/imgui/imgui_impl_bgfx.h"
#include "engine/importer/model_import.h"

#include <SDL.h>
#include <chrono>
#include <thread>

EditorLayer* EditorLayer::s_instance_ = nullptr;

EditorLayer::EditorLayer(RendererBase* renderer) : renderer_(renderer) {
  s_instance_ = this;
}

EditorLayer::~EditorLayer() {
  s_instance_ = nullptr;  // clear on destroy
}

EditorLayer* EditorLayer::Get() {
  return s_instance_;
}

void EditorLayer::Initialize() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  Theme::Initialize();
  LoadIcons();

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

    // ── 0. SDL EVENTS ───────────────────────────────────────────
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);

      if (event.type == SDL_KEYDOWN) {
        Key key = sdl_key_to_key(event);
        if (key != Key::NONE)
          HandleInput(key);
      }

      // ── WINDOW EVENTS ────────────────────────
      if (event.type == SDL_WINDOWEVENT) {
        switch (event.window.event) {
          case SDL_WINDOWEVENT_SIZE_CHANGED:
            if (!WindowManager::IsFullscreen() && !WindowManager::minimized) {
              WindowManager::OnWindowResize(event.window.data1,
                                            event.window.data2);
              ImGui::GetIO().DisplaySize =
                  ImVec2((float)event.window.data1, (float)event.window.data2);
              built_layout_ = false;
            }
            break;

          case SDL_WINDOWEVENT_MINIMIZED:
            WindowManager::minimized = true;
            break;

          case SDL_WINDOWEVENT_RESTORED:
            WindowManager::minimized = false;
            WindowManager::OnWindowResize(event.window.data1,
                                          event.window.data2);
            break;

          default:
            break;
        }
      }

      // toggle biorderless fullscreen with F11
      if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F11)
        WindowManager::ToggleFullscreen();

      if (event.type == SDL_QUIT) {
        quit_ = true;
        editor_exit_pressed();
      }
    }  // end PollEvent loop

    // Skip while window is minimised
    if (WindowManager::minimized) {
      SDL_Delay(16);  // ~60 FPS idle
      continue;
    }

    /* 1 - Clear background and initialize frame */
    bgfx::setViewClear(ViewID::UI_BACKGROUND, BGFX_CLEAR_COLOR, 0x1e1e1eff,
                       1.0f, 0);
    bgfx::touch(ViewID::UI_BACKGROUND);  // process background

    /* 2 - Build ImGui UI structure */
    BeginImGuiFrame(WindowManager::GetWindow());
    RenderUI();

    /* 3 - Finalize ImGui frame definition */
    ImGui::Render();  // prepre imgui draw data (no rendering yet)

    /* 4 - Prepare rendering target */
    auto* graphics = static_cast<GraphicsRenderer*>(renderer_);
    graphics->PrepareFrame();  // set up scene framebuffer view

    /* 5 - Render game content to framebuffer */
    if (is_game_started_) {
      Application::Game()->Render();  // Render 3D scene
    }

    /* 6 - Render ImGui elements */
    ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());  // main window UI

    /* 7 - Handle multi-viewport (detached windows) */
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();

    /* 8 - Submit frame to display */
    renderer_->Render();  // finalize rendering
    bgfx::frame();        // submit to GPU and swap buffers
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
  if (auto* gm = Application::Game()) {
    input_event.pressed_frame_ = gm->GetFrameCount();
  }
  Application::InputSystem()->FowardInputEvent(input_event,
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
  Panels::MenuBar(
      [&]() {
        quit_ = true;
        editor_exit_pressed();
      },
      debug_highlight_ids_, debug_show_metrics_, debug_show_log_,
      debug_activate_picker_, debug_show_style_editor_);

  // ———— Status bar —————
  Panels::StatusBar();
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

    // docked Panels
    ImGui::DockBuilderDockWindow("Toolbar", top);
    ImGui::DockBuilderDockWindow("Hierarchy", left);
    ImGui::DockBuilderDockWindow("Inspector", right);
    ImGui::DockBuilderDockWindow("Scene", dock_id_);
    ImGui::DockBuilderDockWindow("Console", bottom);
    // ImGui::DockBuilderDockWindow(Panels::kDlgWinName, bottom);
    ImGui::DockBuilderDockWindow("Asset Browser", bottom);
    ImGui::DockBuilderDockWindow("Camera", right);

    ImGui::DockBuilderFinish(dock_id_);
    built_layout_ = true;
  }

  //--------------------------- TOP TOOLBAR ----------------------------------
  Panels::ToolbarCallbacks cb{
      .onStart =
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
      .onAddObject =
          [&] {
            // Temporary: Load a 3D model from a hardcoded path
            const std::string gltf_path =
                "C:/Users/moses/Downloads/eiffel_tower/tower.glb";
            GltfImport::LoadModelAndSpawn(gltf_path);
          },
      .onRemoveObject =
          [&] {
            // Temporary stub for now
            Logger::getInstance().Log(LogLevel::Info,
                                      "Remove GameObject not yet implemented");
          },
      .onQuit =
          [&] {
            quit_ = true;
            editor_exit_pressed();
          }};
  ImGui::Begin("Toolbar", nullptr);
  Panels::Toolbar(cb);
  ImGui::End();

  // -------- LEFT : hierarchy + assets -------------------------------------
  static std::vector<std::string> demo_names = {"Camera", "Terrain", "Sun",
                                                "Player"};
  ImGui::Begin("Hierarchy", nullptr);
  Panels::HierarchyPanel(demo_names, "assets");
  ImGui::End();

  // -------- MIDDLE : game view --------------------------------------------
  ImGui::Begin("Scene", nullptr);
  Panels::GameCanvas(is_game_started_, game_canvas_hovered_);
  ImGui::End();

  // -------- RIGHT : inspector ---------------------------------------------
  static std::vector<Panels::Transform> demo_transform(demo_names.size());
  ImGui::Begin("Inspector", nullptr);
  Panels::Inspector(demo_transform);
  ImGui::End();

  //--------------------------- Panels ------------------------------
  ImGui::Begin("Console", nullptr);
  Panels::ConsolePanel();
  ImGui::End();

  // Panels::FileExplorer();

  ImGui::Begin("Asset Browser", nullptr);
  Panels::AssetBrowser();
  ImGui::End();

  // Camera Controls
  ImGui::Begin("Camera", nullptr);
  Panels::CameraControls();
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
  if (debug_show_style_editor_) {
    ImGui::SetNextWindowDockID(0, ImGuiCond_Always);
    ImGui::Begin("Style Editor", &debug_show_style_editor_);
    ImGui::ShowStyleEditor();
    ImGui::End();
  }
  if (debug_activate_picker_) {
    ImGui::DebugStartItemPicker();
    debug_activate_picker_ = false;  // reset
  }
}

void EditorLayer::LoadIcons() {
  tab_icons_.insert({"Hierarchy", ICON_FA_SITEMAP});
  tab_icons_.insert({"Toolbar", ICON_FA_TOOLBOX});
  tab_icons_.insert({"Scene", ICON_FA_GAMEPAD});
  tab_icons_.insert({"Inspector", ICON_FA_LIST});
  tab_icons_.insert({"Console", ICON_FA_TERMINAL});
  tab_icons_.insert({"Camera", ICON_FA_VIDEO});
  tab_icons_.insert({"Assets", ICON_FA_FOLDER_OPEN});
  // tab_icons_.insert({"File Explorer", ICON_FA_FOLDER});
}

void EditorLayer::BeginImGuiFrame(SDL_Window* window) {
  ImGui_ImplSDL2_NewFrame();  // platform backend
  ImGui_Implbgfx_NewFrame();  // renderer backend
  ImGui::NewFrame();          // ImGui begins

  // ---- decorators start here ----
  // drop shadow effect
  // static bool shadowOn = true;
  // ui::shadow::BeginFrame(shadowOn);
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