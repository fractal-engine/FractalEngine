#include "editor_gui.h"

#include <SDL.h>

#include "backends/imgui_impl_sdl2.h"
#include "drivers/imgui_backend.h"

#include <chrono>
#include <thread>
#include "imgui.h"

#include "core/engine_globals.h"
#include "core/logger.h"

#include "renderer/renderer_graphics.h"

#include "subsystem/input/key_map_sdl.h"
#include "subsystem/subsystem_manager.h"

#include "game/game_test.h"

#include "platform/platform_utils.h"

EditorGUI::EditorGUI(std::unique_ptr<RendererBase>& renderer)
    : renderer_(renderer),
      quit_(false),
      is_game_started_(false)  // Track if game is started
{}

EditorGUI::~EditorGUI() {}

void EditorGUI::Run() {
  Logger::getInstance().Log(LogLevel::Info, "EditorGUI main loop start");

  // Get SDL window
  SDL_Window* window = WindowManager::GetWindow();

  // ------------------------------------------------------------
  // 1 - Initialize ImGui
  // ------------------------------------------------------------
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  imgui_backend_.Init();
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2((float)WindowManager::GetWidth(),
                          (float)WindowManager::GetHeight());
  ImGui::StyleColorsDark();

  // Initialize ImGui backend
  platform::InitSDLForImGui(window);

  // ----------------------------------------------------------
  // 2 - Main editor loop
  // ----------------------------------------------------------
  SDL_Event event;
  while (!quit_) {
    // Handle SDL events
    while (SDL_PollEvent(&event)) {
      // Let ImGui process the event first
      ImGui_ImplSDL2_ProcessEvent(&event);

      // Check for key down
      if (event.type == SDL_KEYDOWN) {
        // Convert SDL event to Key enum
        Key key = sdl_key_to_key(event);
        if (key != Key::NONE) {
          // pass event along key handling function
          game_inputed(key);  // Forward key to function
        }
      }

      // Handle window resize events
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        int width = event.window.data1;
        int height = event.window.data2;

        // Notify WindowManager of resize
        WindowManager::OnWindowResize(width, height);

        // Update ImGui display size
        io.DisplaySize = ImVec2((float)width, (float)height);
      }

      // Close window
      if (event.type == SDL_QUIT) {
        Logger::getInstance().Log(LogLevel::Info, "SDL_QUIT event received");
        quit_ = true;
        editor_exit_pressed();  // Signal game to terminate
        break;
      }
    }

    if (quit_)
      break;  // Exit render loop immediately when quitting

    // ----------------------------------------------------------
    // 3 - Prepare frame and start ImGui
    // ----------------------------------------------------------
    // prepare the BGFX frame
    GraphicsRenderer* graphicsRenderer =
        static_cast<GraphicsRenderer*>(renderer_.get());
    graphicsRenderer->PrepareFrame();
    graphicsRenderer->BeginImGuiFrame();

    // ----------------------------------------------------------
    // 4 - ImGui content
    // ----------------------------------------------------------

    // Main window position
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    // Set IMGUI window size using WindowManager
    ImGui::SetNextWindowSize(
        ImVec2(WindowManager::GetWidth(), WindowManager::GetHeight()));

    ImGui::Begin("Fractal Engine", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // Calculate screen space available:
    float totalHeight = ImGui::GetContentRegionAvail().y;

    // 70% for game canvas, 30% for log:
    float canvasHeight = totalHeight * 0.7f;
    float logHeight = totalHeight - canvasHeight;

    /*****************************BUTTONS AREA*****************************/
    // -- Top row: Buttons (Start, Stop, Add, Remove, Quit)
    if (ImGui::Button("Start")) {
      Logger::getInstance().Log(LogLevel::Info, "Editor start game thread");
      is_game_started_ = true;
      game_start_pressed();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
      Logger::getInstance().Log(LogLevel::Info, "Editor stop game thread");
      is_game_started_ = false;
      game_end_pressed();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add GameObject")) {
      // Logic to add a game object
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove GameObject")) {
      // Logic to remove a game object
    }
    ImGui::SameLine();
    if (ImGui::Button("Quit")) {
      Logger::getInstance().Log(LogLevel::Info, "Editor exit on user input");
      quit_ = true;
      editor_exit_pressed();
    }

    ImGui::Separator();

    /*****************************GAME CANVAS AREA*****************************/
    ImGui::BeginChild("GameCanvas", ImVec2(0, canvasHeight), true);

    // Store game canvas state for input processing
    gameCanvasPos_ = ImGui::GetItemRectMin();
    gameCanvasSize_ = ImGui::GetItemRectSize();
    gameCanvasHovered_ = ImGui::IsItemHovered();

    // Get game canvas position and size
    ImVec2 canvasScreenPos = ImGui::GetItemRectMin();
    ImVec2 canvasScreenSize = ImGui::GetItemRectSize();

    // Convert ImVec2 values to framebuffer coordinates (pixels)
    ImVec2 scale = io.DisplayFramebufferScale;
    canvasViewportX = static_cast<uint16_t>(canvasScreenPos.x * scale.x);
    canvasViewportY = static_cast<uint16_t>(canvasScreenPos.y * scale.y);
    canvasViewportW = static_cast<uint16_t>(canvasScreenSize.x * scale.x);
    canvasViewportH = static_cast<uint16_t>(canvasScreenSize.y * scale.y);

    {
      if (is_game_started_) {
        SubsystemManager::GetGameManager()->Render();
      } else {
        ImGui::Text("The game is not started");
      }
    }
    ImGui::EndChild();  // End game canvas area

    /*****************************LOG REGION AREA*****************************/
    // -- Debug Log area (scrollable)
    ImGui::BeginChild("LogRegion", ImVec2(0, 0), true);
    {
      ImGui::Text("Debug Log");
      ImGui::Separator();

      /************** SCROLLABLE REGION **************/
      ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 0), true,
                        ImGuiWindowFlags_HorizontalScrollbar);

      // Grab log entries from Logger
      std::vector<std::string> log_entries =
          Logger::getInstance().GetLogEntries();
      for (auto& line : log_entries) {
        ImGui::TextUnformatted(line.c_str());
      }
      ImGui::EndChild();  // End LogScrollingRegion
    }
    ImGui::EndChild();  // End Debug Log area

    // ImGui::ShowDemoWindow();  // Show ImGui demo window

    /**************************CAMERA CONTROLS*********************************/
    ImGui::BeginChild("CameraControls", ImVec2(300, 0), true);
    {
      ImGui::Text("Camera Controls");
      ImGui::Separator();

      auto* game = dynamic_cast<GameTest*>(
          SubsystemManager::GetGameManager()->GetGame());
      if (game) {
        ImGui::SliderFloat3("Eye Position", game->cameraEye, -200.0f, 200.0f);
        ImGui::SliderFloat3("Look At", game->cameraAt, -200.0f, 200.0f);
        ImGui::SliderFloat3("Up Vector", game->cameraUp, -1.0f, 1.0f);
        ImGui::SliderFloat("FOV", &game->cameraFOV, 10.0f, 120.0f);

        if (ImGui::Button("Reset Camera")) {
          game->cameraEye[0] = 120.0f;
          game->cameraEye[1] = 60.0f;
          game->cameraEye[2] = 32.0f;

          game->cameraAt[0] = 32.0f;
          game->cameraAt[1] = 0.0f;
          game->cameraAt[2] = 32.0f;

          game->cameraUp[0] = 1.0f;
          game->cameraUp[1] = 0.0f;
          game->cameraUp[2] = 0.0f;

          game->cameraFOV = 80.0f;
        }
      }
    }
    ImGui::EndChild();  // End CameraControls region

    ImGui::End();  // End main window
    // ----------------------------------------------------------
    // 5 - Render ImGui
    // ----------------------------------------------------------
    // Submit one frame (delegated to Renderer)
    renderer_->Render();

    graphicsRenderer->EndImGuiFrame();  // End ImGui frame

    bgfx::frame();  // Submit everything to GPU

    // TODO: Add a delay or vsync depending on FPS cap
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  Logger::getInstance().Log(LogLevel::Info, "EditorGUI main loop exited");
}

void EditorGUI::Shutdown() {
  Logger::getInstance().Log(LogLevel::Info, "Shutting down EditorGUI");
  imgui_backend_.Shutdown();
}

void EditorGUI::RequestUpdate() {
  /* In SDL, we redraw continuously.
  This can be left empty or used to post a custom event. */
}

// -----------------------------------------------------------------
//  This function receives the Key from sdl_key_to_key() and either
//  handles 0/1/2 or forwards WASD to input system, so that game_test can see
//  it.
// -----------------------------------------------------------------
// Handles keyboard input
void EditorGUI::game_inputed(Key key) {
  Logger::getInstance().Log(
      LogLevel::Debug,
      "[EditorGUI] Key pressed: " + std::to_string(static_cast<int>(key)));

  // Bypass WantCaptureKeyboard if game is active and canvas is hovered
  if (ImGui::GetIO().WantCaptureKeyboard && !gameCanvasHovered_) {
    return;
  }

  // Handle numeric shortcuts for Start/Stop/Quit
  switch (key) {
    case Key::DIGIT_0:  // '0' for Quit
      Logger::getInstance().Log(LogLevel::Info,
                                "[EditorGUI] Received '0' => Quit");
      quit_ = true;
      editor_exit_pressed();
      return;
    case Key::DIGIT_1:  // '1' for Start Game
      Logger::getInstance().Log(LogLevel::Info,
                                "[EditorGUI] Received '1' => Start game");
      if (!is_game_started_) {
        is_game_started_ = true;
        game_start_pressed();
      }
      return;
    case Key::DIGIT_2:  // '2' for Stop Game
      Logger::getInstance().Log(LogLevel::Info,
                                "[EditorGUI] Received '2' => Stop game");
      if (is_game_started_) {
        is_game_started_ = false;
        game_end_pressed();
      }
      return;
    default:
      break;
  }

  // Early return if game is not started
  if (!is_game_started_)
    return;

  // For keys such as WASD (or others) used to move the game object,
  // forward them to the input system so that the game update (GameTest::Update)
  // can detect them via IsJustPressed.
  Logger::getInstance().Log(LogLevel::Debug,
                            "[EditorGUI] Forwarding key to game system");

  InputEvent input_event(key);
  // Set the frame count from the Game Manager.
  if (auto* gm = SubsystemManager::GetGameManager().get()) {
    input_event.pressed_frame_ = gm->GetFrameCount();
  }
  // Forward event to the centralized input system.
  SubsystemManager::GetInput()->FowardInputEvent(input_event,
                                                 input_event.pressed_frame_);
}
