#include "editor/editor_tui.h"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include "audio/sound_manager.h"
#include "core/logger.h"
#include "editor/scroller.hpp"
#include "subsystem/input/key_map_ftxui.h"

using namespace ftxui;

// Custom button rendering function
ButtonOption CustomButtonOption() {
  ButtonOption option;
  option.transform = [](const EntryState& s) {
    auto element = text(s.label);
    // When focused, display text in black with a white background.
    if (s.focused) {
      element = element | color(Color::Black) | bgcolor(Color::White);
    } else {
      // Otherwise, display text in white.
      element = element | color(Color::White);
    }
    // Use hbox() to horizontally group the button text and a fixed gap.
    element = hbox({element, text("   ") | size(WIDTH, EQUAL, 3) | notflex});
    return element;
  };
  return option;
}

EditorTUI::EditorTUI(const std::unique_ptr<RendererBase>& renderer)
    : screen_(ScreenInteractive::Fullscreen()),
      game_canvas_(dynamic_cast<RendererText&>(*renderer).GetGameCanvas()),
      game_interactive_(CatchEvent(game_canvas_,
                                   [&](Event event) {
                                     if (event.is_character()) {
                                       game_inputed(ftxui_key_to_key(event));
                                       return true;
                                     }
                                     return false;
                                   })),
      game_canvas_height_(100),
      game_canvas_width_(100),
      scroll_x(0.0),
      scroll_y(0.0)  // These are no longer used by the scroller, but kept if
                     // needed elsewhere.
{
  // Initialize control buttons.
  auto start_button = Button(
      "Start",
      [&] {
        Logger::getInstance().Log(LogLevel::Info, "Editor start game thread");
        SoundManager::Instance().playClickAsync();
        game_start_pressed();
        is_game_start_ = true;
      },
      CustomButtonOption());

  auto stop_button = Button(
      "Stop",
      [&] {
        Logger::getInstance().Log(LogLevel::Info, "Editor stop game thread");
        SoundManager::Instance().playClickAsync();
        is_game_start_ = false;
        game_end_pressed();
      },
      CustomButtonOption());

  auto add_gameobject_button = Button(
      "Add GameObject",
      [] {
        // Logic to add a game object.
      },
      CustomButtonOption());

  auto remove_gameobject_button = Button(
      "Remove GameObject",
      [] {
        // Logic to remove a game object.
      },
      CustomButtonOption());

  auto quit_button = Button(
      "Quit",
      [&] {
        Logger::getInstance().Log(LogLevel::Info, "Editor exit on user input");
        editor_exit_pressed();
        screen_.ExitLoopClosure()();
      },
      CustomButtonOption());

  control_buttons_ =
      Container::Horizontal({start_button, stop_button, add_gameobject_button,
                             remove_gameobject_button, quit_button});

  // The Scroller component will handle scrolling of the debug log.

  editor_layout_ = std::move(renderLayout());
  editor_interactive_ = std::move(addInteraction());
}

Component EditorTUI::renderLayout() {
  // Create the container for your control buttons.
  auto control_container = Container::Vertical({
      control_buttons_,
  });

  // Build a renderer for your debug log content.
  auto debug_renderer = Renderer([this] {
    // Update log entries so they're current.
    ReadLogFile();

    ftxui::Elements debug_elements;  // Elements to display in the debug log.
    for (const std::string& entry : log_entries_) {
      debug_elements.push_back(text(entry));
    }
    // Build the vbox for the log entries, then frame.
    return vbox(std::move(debug_elements)) | frame;
  });

  // Wrap the debug_renderer in a Scroller so it can scroll.
  auto scrollable_debug = Scroller(debug_renderer);

  // Create a container for the debug log.
  auto debug_container = Container::Vertical({
      scrollable_debug,
  });

  // Create the final renderer that arranges everything on screen.
  auto main_renderer =
      Renderer(game_interactive_, [this, control_container, debug_container] {
        // Decide what to show for the game canvas.
        Element game_canvas_element =
            is_game_start_
                ? window(text("Game Canvas"),
                         game_canvas_->Render() |
                             size(HEIGHT, EQUAL, game_canvas_height_) |
                             size(WIDTH, EQUAL, game_canvas_width_))
                : window(text("Game Canvas"),
                         center(text("The game is not started")) |
                             size(HEIGHT, EQUAL, game_canvas_height_) |
                             size(WIDTH, EQUAL, game_canvas_width_));

        // Create an Element for the debug window.
        auto debug_element =
            window(text("Debug Log"),
                   debug_container->Render() |
                       size(HEIGHT, EQUAL, 50));  // Change size of debug log

        return window(
            text("Fractal Engine"),
            vbox({
                control_container->Render(),                      // Buttons
                hbox({filler(), game_canvas_element, filler()}),  // Game canvas
                debug_element  // Debug log window
            }));
      });

  // Ensure that event handling is directed to both control_container and
  // debug_container
  return CatchEvent(main_renderer, [this, control_container, debug_container,
                                    main_renderer](Event event) {
    if (control_container->OnEvent(event) || debug_container->OnEvent(event)) {
      return true;
    }
    if (event == Event::Character('0')) {
      editor_exit_pressed();
      Logger::getInstance().Log(LogLevel::Info, "Editor exit on user input");
      screen_.ExitLoopClosure()();
      return true;
    }
    if (event == Event::Character('1')) {
      Logger::getInstance().Log(LogLevel::Info, "Editor start game thread");
      game_start_pressed();
      is_game_start_ = true;
      return true;
    }
    if (event == Event::Character('2')) {
      Logger::getInstance().Log(LogLevel::Info, "Editor stop game thread");
      is_game_start_ = false;
      game_end_pressed();
      return true;
    }
    return main_renderer->OnEvent(event);
  });
}

void EditorTUI::ReadLogFile() {
  // Read the log file and update log_entries_.
  Logger& logger = Logger::getInstance();
  log_entries_ = logger.GetLogEntries();
}

Component EditorTUI::addInteraction() {  // we need this as backup so I'm
                                         // keeping keyboard functionality
  return CatchEvent(editor_layout_, [&](Event event) {
    if (event == Event::Character('0')) {
      editor_exit_pressed();
      Logger::getInstance().Log(LogLevel::Info, "Editor exit on user input");
      screen_.ExitLoopClosure()();
      return true;
    }
    if (event == Event::Character('1')) {
      Logger::getInstance().Log(LogLevel::Info, "Editor start game thread");
      game_start_pressed();
      is_game_start_ = true;
      return true;
    }
    if (event == Event::Character('2')) {
      Logger::getInstance().Log(LogLevel::Info, "Editor stop game thread");
      is_game_start_ = false;
      game_end_pressed();
      return true;
    }
    // Forward other events to the layout for handling by buttons.
    return editor_layout_->OnEvent(event);
  });
}

void EditorTUI::Run() {
  // Rebuild layout and interaction components as needed.
  editor_layout_ = renderLayout();
  editor_interactive_ = addInteraction();

  Logger::getInstance().Log(LogLevel::Info, "Editor main loop start");
  screen_.Loop(editor_interactive_);
  Logger::getInstance().Log(LogLevel::Info, "Editor main loop passed");
}

void EditorTUI::RequestUpdate() {
  screen_.PostEvent(Event::Custom);
}
