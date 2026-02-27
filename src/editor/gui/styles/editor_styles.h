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
  // #141414 - Main window background (Deep Black)
  static constexpr ImU32 background = IM_COL32(20, 20, 20, 255);

  // #E6E6E6 - Standard text
  static constexpr ImU32 text = IM_COL32(230, 230, 230, 255);
  // #E6E6E6B4 - Transparent text
  static constexpr ImU32 text_transparent = IM_COL32(230, 230, 230, 180);

  // #2E2E2E82 - Transparent elements
  static constexpr ImU32 element_transparent = IM_COL32(46, 46, 46, 130);
  static constexpr ImU32 element_hovered_transparent_overlay =
      IM_COL32(255, 255, 255, 10);
  static constexpr ImU32 element_active_transparent_overlay =
      IM_COL32(255, 255, 255, 35);

  // #2966C0 - Vibrant blue highlight against black
  static constexpr ImU32 selection = IM_COL32(41, 102, 192, 255);
  // #333333 - Inactive selection grey
  static constexpr ImU32 selection_inactive = IM_COL32(51, 51, 51, 255);

  // #2E2E2E - Base custom element (buttons/headers)
  static constexpr ImU32 element = IM_COL32(46, 46, 46, 255);
  // #404040 - Hovered custom element
  static constexpr ImU32 element_hovered = IM_COL32(64, 64, 64, 255);
  // #2966C0 - Active custom element
  static constexpr ImU32 element_active = IM_COL32(41, 102, 192, 255);
  // #262626 - Component header
  static constexpr ImU32 element_component = IM_COL32(38, 38, 38, 255);

  // #404040 - Standard border (Lightened for contrast)
  static constexpr ImU32 border_color = IM_COL32(64, 64, 64, 255);
  // #1F1F1F - Tab background
  static constexpr ImU32 tab_color = IM_COL32(31, 31, 31, 255);
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