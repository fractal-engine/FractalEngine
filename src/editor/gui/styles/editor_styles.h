#ifndef EDITOR_STYLES_H
#define EDITOR_STYLES_H

#include "../../vendor/IconFontCppHeaders/IconsFontAwesome6.h"

#include "imgui.h"

namespace EditorStyles {

struct Fonts {
  // Paragraph
  ImFont* p = nullptr;
  ImFont* p_bold = nullptr;

  // Headings
  ImFont* h1 = nullptr;
  ImFont* h1_bold = nullptr;
  ImFont* h2 = nullptr;
  ImFont* h2_bold = nullptr;
  ImFont* h3 = nullptr;
  ImFont* h3_bold = nullptr;
  ImFont* h4 = nullptr;
  ImFont* h4_bold = nullptr;

  // Small
  ImFont* s = nullptr;
  ImFont* s_bold = nullptr;

  // Special
  ImFont* console = nullptr;
  ImFont* icons = nullptr;
};

// Access fonts
const Fonts& GetFonts();

void LoadFonts(ImGuiIO& io);
void SetupStyle();
void Initialize();

}  // namespace EditorStyles

// FONT PATHS
struct EditorFontPath {
  // Switch from Monospace to a clean Sans-Serif font (Inter)
  static constexpr const char* regular = "resources/fonts/Inter-Regular.ttf";
  static constexpr const char* bold = "resources/fonts/Inter-SemiBold.ttf";
  static constexpr const char* icons = "resources/fonts/fa-solid-900.ttf";

  // Console font changed to FiraCode for better readability
  static constexpr const char* console = "resources/fonts/FiraCode-Regular.ttf";
};

// DRAW LIST COLOR CONSTANTS
struct EditorColor {
  // #383838 - Main window background
  static constexpr ImU32 background = IM_COL32(56, 56, 56, 255);

  // #D2D2D2 - Standard text
  static constexpr ImU32 text = IM_COL32(210, 210, 210, 255);
  // #D2D2D2D2 - Transparent text
  static constexpr ImU32 text_transparent = IM_COL32(210, 210, 210, 210);

  // #45454582 - Transparent elements
  static constexpr ImU32 element_transparent = IM_COL32(69, 69, 69, 130);
  static constexpr ImU32 element_hovered_transparent_overlay =
      IM_COL32(255, 255, 255, 10);
  static constexpr ImU32 element_active_transparent_overlay =
      IM_COL32(255, 255, 255, 35);

  // #2C5D87 - Unity highlight blue
  static constexpr ImU32 selection = IM_COL32(44, 93, 135, 255);
  // #4D4D4D - Inactive selection grey
  static constexpr ImU32 selection_inactive = IM_COL32(77, 77, 77, 255);

  // #454545 - Base custom element (buttons/headers)
  static constexpr ImU32 element = IM_COL32(69, 69, 69, 255);
  // #595959 - Hovered custom element
  static constexpr ImU32 element_hovered = IM_COL32(89, 89, 89, 255);
  // #2C5D87 - Active custom element (Unity Blue)
  static constexpr ImU32 element_active = IM_COL32(44, 93, 135, 255);
  // #3E3E3E - Component header
  static constexpr ImU32 element_component = IM_COL32(62, 62, 62, 255);

  // #1A1A1A - Standard border
  static constexpr ImU32 border_color = IM_COL32(26, 26, 26, 255);
  // #282828 - Tab background
  static constexpr ImU32 tab_color = IM_COL32(40, 40, 40, 255);
};

// SIZING CONSTANTS
struct EditorSizes {
  // Font sizes
  static constexpr float p_font_size =
      15.0f;  // Bumped to 15 for better readability
  static constexpr float h1_font_size =
      24.0f;  // Smoothed out heading hierarchy
  static constexpr float h2_font_size = 20.0f;
  static constexpr float h3_font_size = 17.0f;
  static constexpr float h4_font_size = 15.0f;
  static constexpr float s_font_size = 13.0f;
  static constexpr float console_font_size = 13.0f;  // Bumped slightly

  // Icon sizes (Scaled to match the new text baseline, prevents UI stretching)
  static constexpr float p_icon_size =
      16.0f;  // Reduced from 22 to align with 15px text
  static constexpr float p_bold_icon_size = 16.0f;
  static constexpr float h2_icon_size = 22.0f;
  static constexpr float h2_bold_icon_size = 22.0f;
  static constexpr float s_icon_size = 14.0f;
  static constexpr float s_bold_icon_size = 14.0f;

  // Layout
  static constexpr float window_padding =
      8.0f;  // Reduced from 30.0f
  static constexpr float frame_padding = 4.0f;
};

#endif  // EDITOR_STYLES_H