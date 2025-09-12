#include "editor/editor_ui.h"
#include "editor/resources/theme/dark_theme.hpp"
#include "editor/runtime/runtime.h"
#include "engine/core/engine_globals.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "engine/ecs/world.h"
#include "gui/inspector_panel.h"
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

#include <SDL.h>
#include <chrono>
#include <thread>

EditorUI* EditorUI::s_instance_ = nullptr;

EditorUI::EditorUI(RendererBase* renderer) : renderer_(renderer) {
  s_instance_ = this;
}

EditorUI::~EditorUI() {
  s_instance_ = nullptr;  // clear on destroy
}

EditorUI* EditorUI::Get() {
  return s_instance_;
}

// TODO: refactor loop, should be split into EditorUI::NewFrame() /
// EditorUI::Render() Renamed to NextFrame()??

// --- NEW IMPLEMENTATIONS ---
// The logic from the old, monolithic Run() method has been split into the
// following functions. This allows the main loop in `runtime.cpp` to
// orchestrate the frame execution step-by-step, which is essential for
// the new FrameGraph-based rendering architecture.

// ---- Pending approval by Louis Mercier ----

void EditorUI::Initialize() {
    // This function contains the one-time setup logic that was previously
    // at the very beginning of the EditorUI::Run() method.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    Theme::Initialize();
    LoadIcons();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigViewportsNoAutoMerge = true;
    io.ConfigViewportsNoTaskBarIcon = true;

    // Initialize the ImGui backends for SDL2 and BGFX.
    ImGui_Implbgfx_Init(ViewID::UI);
    // ApplyImGuiShaders(); // This should be called here if it exists
    ImGui_ImplSDL2_InitForOther(WindowManager::GetWindow());

    io.DisplaySize = ImVec2((float)WindowManager::GetWidth(), (float)WindowManager::GetHeight());
    io.DisplayFramebufferScale = ImVec2(WindowManager::GetDPIScale(), WindowManager::GetDPIScale());
}

void EditorUI::HandleEvent(const SDL_Event& event) {
    // --- RETAINED FROM OLD Run() METHOD ---
    // This function encapsulates all event processing logic that was previously
    // inside the `while (SDL_PollEvent(&event))` loop in the old Run() method.
    // No logic has been removed, only relocated.
    ImGui_ImplSDL2_ProcessEvent(&event);

    if (event.type == SDL_KEYDOWN) {
        Key key = sdl_key_to_key(event);
        if (key != Key::NONE)
            HandleInput(key);
    }

    if (event.type == SDL_WINDOWEVENT) {
        switch (event.window.event) {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            if (!WindowManager::IsFullscreen() && !WindowManager::minimized) {
                WindowManager::OnWindowResize(event.window.data1, event.window.data2);
                ImGui::GetIO().DisplaySize = ImVec2((float)event.window.data1, (float)event.window.data2);
                built_layout_ = false;
            }
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
            WindowManager::minimized = true;
            break;
        case SDL_WINDOWEVENT_RESTORED:
            WindowManager::minimized = false;
            WindowManager::OnWindowResize(event.window.data1, event.window.data2);
            break;
        default:
            break;
        }
    }

    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F11)
        WindowManager::ToggleFullscreen();

    if (event.type == SDL_QUIT) {
        quit_ = true; // Still useful for internal state checks
        editor_exit_pressed(); // Signal Runtime to orchestrate a clean shutdown
    }
}

void EditorUI::BeginFrame() {
    // This function kicks off a new ImGui frame. It contains the calls that
    // used to be at the top of the main `while(!quit_)` loop in Run().
    ImGui_ImplSDL2_NewFrame();
    ImGui_Implbgfx_NewFrame();
    ImGui::NewFrame();
}

void EditorUI::RenderPanels() {
    // This is the primary UI drawing step. It simply calls the main RenderUI()
    // function, which is responsible for defining all ImGui windows and widgets.
    RenderUI();
}

void EditorUI::RenderDraws() {
    // This function handles the final step of rendering the UI. It takes the draw
    // data prepared by ImGui::Render() and submits it to the BGFX backend.
    // This logic was previously at the end of the Run() method.
    ImGui::Render();
    ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
}

void EditorUI::Destroy() {
    // Contains the shutdown logic from the end of the old Run() or destructor.
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
  auto& ecs = ECS::Main();

  // -------- LEFT : HIERARCHY (Now reads live data from ECS) --------
  ImGui::Begin("Hierarchy", nullptr);
  Panels::HierarchyPanel();  // We will also update this panel to be data-driven
  ImGui::End();

  // -------- MIDDLE : SCENE --------
  // The rendering pipeline now handles the gizmo, so this panel is just a
  // simple canvas.
  ImGui::Begin("Scene", nullptr);
    // The GraphicsRenderer no longer knows about a "scene texture". That responsibility
    // has moved to the FrameGraph, which manages all rendering targets (attachments).
    // This decouples the UI from the low-level renderer's internal state.

    // Now we have a new method as follows:
    // We now ask the FrameGraph for the final "scene_color" attachment by its logical name.
    bgfx::TextureHandle scene_texture_handle = Runtime::GetFrameGraph().GetAttachmentTexture("scene_color");
    ImTextureID scene_tex_id = (ImTextureID)0;

    // We must check if the handle is valid. It might be invalid on the very first
    // frame or during a resize operation before the new texture is created.
    if (bgfx::isValid(scene_texture_handle)) {
        // Convert the BGFX handle to an ImTextureID that ImGui's Image() function can use.
        scene_tex_id = (ImTextureID)(uintptr_t)scene_texture_handle.idx;
    }
    
    // Now we can use the retrieved texture ID in the GameCanvas panel.
    // We now pass the texture ID directly into the GameCanvas panel. This makes
    // the panel more self-contained and easier to reuse.
    Panels::GameCanvas(is_game_started_, game_canvas_hovered_, scene_tex_id);

    ImGui::End();


 // -------- RIGHT : INSPECTOR (now calls the new panel) -----------------
  ImGui::Begin("Inspector", nullptr);

  Entity selectedEntity = GetSelectedEntity();
  if (selectedEntity != entt::null && ecs.Reg().valid(selectedEntity)) {
    // 1. Get the TransformComponent from the selected entity.
    auto& transform = ecs.Get<TransformComponent>(selectedEntity);

    // 2. Call the newly designed Inspector panel with the live component.
    Panels::Inspector(transform);
  } else {
    ImGui::TextDisabled("Select an entity to inspect its components.");
  }

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

void EditorUI::LoadIcons() {
  tab_icons_.insert({"Hierarchy", ICON_FA_SITEMAP});
  tab_icons_.insert({"Toolbar", ICON_FA_TOOLBOX});
  tab_icons_.insert({"Scene", ICON_FA_GAMEPAD});
  tab_icons_.insert({"Inspector", ICON_FA_LIST});
  tab_icons_.insert({"Console", ICON_FA_TERMINAL});
  tab_icons_.insert({"Camera", ICON_FA_VIDEO});
  tab_icons_.insert({"Assets", ICON_FA_FOLDER_OPEN});
  // tab_icons_.insert({"File Explorer", ICON_FA_FOLDER});
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