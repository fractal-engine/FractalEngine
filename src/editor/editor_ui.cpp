#include "editor_ui.h"

#include "editor/gui/styles/editor_styles.h"
#include "editor/runtime/runtime.h"

#include "engine/context/engine_context.h"
#include "engine/core/engine_globals.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "engine/ecs/world.h"

#include "gui/asset_browser.h"
#include "gui/camera_controls.h"
#include "gui/console_panel.h"
#include "gui/file_explorer.h"
#include "gui/game_canvas.h"
#include "gui/hierarchy_panel.h"
#include "gui/inspector_panel.h"
#include "gui/menu_bar.h"
#include "gui/search/search_popup.h"
#include "gui/status_bar.h"
#include "gui/styles/editor_styles.h"
#include "gui/terrain_editor.h"
#include "gui/toolbar.h"
#include "gui/world_settings.h"

#include "platform/platform_utils.h"
#include "platform/window_manager.h"

#include "resources/decorators/drop_shadows.h"

#include <backends/imgui_impl_sdl2.h>
#include "editor/vendor/imgui/imgui_impl_bgfx.h"
#include "imgui_internal.h"
#include "imgui_utils.h"

#include <SDL.h>

EditorUI* EditorUI::s_instance_ = nullptr;
// uint32_t EditorUI::g_id_counter_ = 0;

EditorUI::EditorUI(RendererBase* renderer) : renderer_(renderer) {
  s_instance_ = this;
}

EditorUI::~EditorUI() {
  s_instance_ = nullptr;  // clear on destroy
}

EditorUI* EditorUI::Get() {
  return s_instance_;
}

void EditorUI::Initialize() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  EditorStyles::Initialize();
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

// TODO: refactor loop, should be split into EditorUI::NewFrame() /
// EditorUI::Render() Renamed to NextFrame()??
// Also create a window_viewport for SDL stuff in the loop
void EditorUI::Run() {
  Logger::getInstance().Log(LogLevel::Info, "EditorUI main loop start");

  Initialize();

  SDL_Event event;
  while (!quit_) {

    // TODO: move this out of the loop later
    EngineContext::NextFrame();

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

    // Process system events
    Runtime::Project().PollEvents();

    /* 1 - Clear background and initialize frame */
    bgfx::setViewClear(ViewID::UI_BACKGROUND, BGFX_CLEAR_COLOR, 0x1e1e1eff,
                       1.0f, 0);
    bgfx::touch(ViewID::UI_BACKGROUND);  // process background

    /* 2 - Build ImGui UI structure */
    BeginImGuiFrame(WindowManager::GetWindow());

    /* 3 - Prepare rendering target */
    auto* graphics = static_cast<GraphicsRenderer*>(renderer_);
    graphics->PrepareFrame();  // set up scene framebuffer view

    // Update global resources and set them in frame graph
    // TODO: check if this should be called here
    Runtime::UpdateGlobalResources();

    Runtime::GetFrameGraph().SetGlobalResources(
        Runtime::BuildGlobalResources());

    Runtime::GetSceneViewPipeline().Render();

    RenderUI();

    /* 4 - Finalize ImGui frame definition */
    ImGui::Render();  // prepare imgui draw data (no rendering yet)

    /* 5 - Render game content to framebuffer */
    if (is_game_started_) {
      Runtime::Game()->Render();  // Render 3D scene
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
  Logger::getInstance().Log(LogLevel::Info, "EditorUI loop exited");
}

void EditorUI::RequestUpdate() {}

void EditorUI::Destroy() {
  Logger::getInstance().Log(LogLevel::Info, "Shutting down EditorUI");
  ImGui_Implbgfx_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void EditorUI::HandleInput(Key key) {
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
  if (auto* gm = Runtime::Game()) {
    input_event.pressed_frame_ = gm->GetFrameCount();
  }
  Runtime::InputDevice()->ForwardInputEvent(input_event,
                                            input_event.pressed_frame_);
}

void EditorUI::DockSpace() {
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

void EditorUI::RenderUI() {
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
    ImGui::DockBuilderDockWindow("World", right);
    ImGui::DockBuilderDockWindow("Terrain Editor", right);
    ImGui::DockBuilderDockWindow("Scene", dock_id_);
    ImGui::DockBuilderDockWindow("Console", bottom);
    // ImGui::DockBuilderDockWindow(Panels::kDlgWinName, bottom);
    ImGui::DockBuilderDockWindow("Asset Browser", bottom);
    ImGui::DockBuilderDockWindow("Camera", right);

    ImGui::DockBuilderFinish(dock_id_);
    built_layout_ = true;
  }

  //--------------------------- TOP TOOLBAR ----------------------------------
  Panels::ToolbarCallbacks cb{.onStart =
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
  Panels::Toolbar(cb);
  ImGui::End();

  // Get the ECS instance once for this frame
  auto& world = ECS::Main();

  // -------- LEFT : HIERARCHY (Now reads live data from ECS) --------
  ImGui::Begin("Hierarchy", nullptr);
  Panels::HierarchyPanel();  // We will also update this panel to be data-driven
  ImGui::End();

  // -------- MIDDLE : SCENE --------
  // The rendering pipeline now handles the gizmo, so this panel is just a
  // simple canvas.
  ImGui::Begin("Scene", nullptr);
  Panels::GameCanvas(is_game_started_, game_canvas_hovered_,
                     game_canvas_focused_);
  UpdateMovement();
  ImGui::End();

  // -------- RIGHT : INSPECTOR -----------------
  ImGui::Begin("Inspector", nullptr);
  Entity selectedEntity = GetSelectedEntity();
  if (selectedEntity != entt::null && world.Reg().valid(selectedEntity)) {
    // 1. Get the TransformComponent from the selected entity.
    auto& transform = world.Get<TransformComponent>(selectedEntity);

    // 2. Call the newly designed Inspector panel with the live component.
    Panels::Inspector(transform);
    Panels::InspectVolume(selectedEntity);

  } else {
    ImGui::TextDisabled("Select an entity to inspect its components.");
  }
  ImGui::End();

  // -------- RIGHT : WORLD SETTINGS --------
  ImGui::Begin("World", nullptr);
  Panels::WorldSettings();
  ImGui::End();

  // -------- RIGHT : TERRAIN EDITOR --------
  ImGui::Begin("Terrain Editor", nullptr);
  Panels::TerrainEditor();
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

  //------------------------- SEARCH POPUP --------------------------
  SearchPopup::Render();

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

void EditorUI::LoadIcons() {
  tab_icons_.insert({"Hierarchy", ICON_FA_SITEMAP});
  tab_icons_.insert({"Toolbar", ICON_FA_TOOLBOX});
  tab_icons_.insert({"Scene", ICON_FA_GAMEPAD});
  tab_icons_.insert({"Inspector", ICON_FA_LIST});
  tab_icons_.insert({"Console", ICON_FA_TERMINAL});
  tab_icons_.insert({"Camera", ICON_FA_VIDEO});
  tab_icons_.insert({"Assets", ICON_FA_FOLDER_OPEN});
  tab_icons_.insert({"World", ICON_FA_GLOBE});
  // tab_icons_.insert({"File Explorer", ICON_FA_FOLDER});
}

void EditorUI::BeginImGuiFrame(SDL_Window* window) {

  // Reset ID counter for new frame
  g_id_counter_ = 0;

  ImGui_ImplSDL2_NewFrame();  // platform backend
  ImGui_Implbgfx_NewFrame();  // renderer backend
  ImGui::NewFrame();          // ImGui begins
  ImGuizmo::BeginFrame();     // ImGuizmo begins

  // ---- decorators start here ----
  // drop shadow effect
  // static bool shadowOn = true;
  // ui::shadow::BeginFrame(shadowOn);
}

// ── Selection API ─────────────────────────────────────────────────────────
void EditorUI::SetSelectedEntity(Entity entity) {
  selected_entity_ = entity;
  last_selected_entity_ = entity;
}

Entity EditorUI::GetSelectedEntity() const {
  return selected_entity_;
}

Entity EditorUI::GetLastSelectedEntity() const {
  return last_selected_entity_;
}

// TODO: move this to viewport
void EditorUI::UpdateMovement() {
  // Build per-frame input
  GodCameraFrameInput input{};
  input.scene_hovered = game_canvas_hovered_;

  // Enable/disable text input
  if (input.scene_hovered) {
    Platform::DisableTextInput();
  } else {
    Platform::EnableTextInput();
  }

  input.right_mouse =
      input.scene_hovered && ImGui::IsMouseDown(ImGuiMouseButton_Right);
  input.middle_mouse =
      input.scene_hovered && ImGui::IsMouseDown(ImGuiMouseButton_Middle);

  ImGuiIO& io = ImGui::GetIO();
  input.mouse_delta = (input.right_mouse || input.middle_mouse)
                          ? glm::vec2(io.MouseDelta.x, io.MouseDelta.y)
                          : glm::vec2(0.0f);
  input.mouse_wheel = io.MouseWheel;

  // WASD axis
  if (input.scene_hovered) {
    const float fwd = (ImGui::IsKeyDown(ImGuiKey_W) ? 1.0f : 0.0f) -
                      (ImGui::IsKeyDown(ImGuiKey_S) ? 1.0f : 0.0f);
    const float sid = (ImGui::IsKeyDown(ImGuiKey_D) ? 1.0f : 0.0f) -
                      (ImGui::IsKeyDown(ImGuiKey_A) ? 1.0f : 0.0f);
    input.move_axis = glm::vec2(fwd, sid);

    // Arrow keys for camera rotation
    const float look_y = (ImGui::IsKeyDown(ImGuiKey_UpArrow) ? 1.0f : 0.0f) -
                         (ImGui::IsKeyDown(ImGuiKey_DownArrow) ? 1.0f : 0.0f);
    const float look_x = (ImGui::IsKeyDown(ImGuiKey_RightArrow) ? 1.0f : 0.0f) -
                         (ImGui::IsKeyDown(ImGuiKey_LeftArrow) ? 1.0f : 0.0f);
    input.look_axis = glm::vec2(look_x, look_y);

    // Speed boost with SHIFT
    input.hold_shift = ImGui::IsKeyDown(ImGuiKey_LeftShift) ||
                       ImGui::IsKeyDown(ImGuiKey_RightShift);
  } else {
    input.move_axis = glm::vec2(0.0f);
    input.look_axis = glm::vec2(0.0f);
    input.hold_shift = false;
  }

  // Get the ECS transform
  auto& pipeline = Runtime::GetSceneViewPipeline();
  TransformComponent& CameraTransform = std::get<0>(pipeline.GetGodCamera());

  // Run update
  GodCameraUpdateMovement(god_state_, CameraTransform, input);

  // Rebuild model matrix
  Transform::Evaluate(CameraTransform);
}

ImGuiID EditorUI::GenerateId() {
  return ++g_id_counter_;
}

std::string EditorUI::GenerateIdString() {
  return "##" + std::to_string(++g_id_counter_);
}
