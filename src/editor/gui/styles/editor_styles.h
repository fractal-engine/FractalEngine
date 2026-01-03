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
  static constexpr const char* regular =
      "resources/fonts/NotoSansMono_Condensed-Regular.ttf";
  static constexpr const char* bold =
      "resources/fonts/NotoSansMono_Condensed-SemiBold.ttf";
  static constexpr const char* icons = "resources/fonts/fa-solid-900.ttf";
  static constexpr const char* console =
      "resources/fonts/TerminusTTF-4.49.3.ttf";
};

// DRAW LIST COLOR CONSTANTS
struct EditorColor {
  static constexpr ImU32 background = IM_COL32(16, 16, 16, 255);

  static constexpr ImU32 text = IM_COL32(255, 255, 255, 255);
  static constexpr ImU32 text_transparent = IM_COL32(255, 255, 255, 210);

  static constexpr ImU32 element_transparent = IM_COL32(80, 80, 80, 130);
  static constexpr ImU32 element_hovered_transparent_overlay =
      IM_COL32(255, 255, 255, 10);
  static constexpr ImU32 element_active_transparent_overlay =
      IM_COL32(255, 255, 255, 35);

  static constexpr ImU32 selection = IM_COL32(85, 125, 255, 255);
  static constexpr ImU32 selection_inactive = IM_COL32(100, 100, 100, 255);

  static constexpr ImU32 element = IM_COL32(51, 51, 51, 255);
  static constexpr ImU32 element_hovered = IM_COL32(64, 64, 64, 255);
  static constexpr ImU32 element_active = IM_COL32(77, 77, 77, 255);
  static constexpr ImU32 element_component = IM_COL32(178, 178, 178, 255);

  static constexpr ImU32 border_color = IM_COL32(0, 0, 0, 255);
  static constexpr ImU32 tab_color = IM_COL32(38, 38, 38, 255);
};

// SIZING CONSTANTS
struct EditorSizes {
  // Font sizes
  static constexpr float p_font_size = 14.0f;
  static constexpr float h1_font_size = 26.0f;
  static constexpr float h2_font_size = 22.0f;
  static constexpr float h3_font_size = 17.0f;
  static constexpr float h4_font_size = 15.0f;
  static constexpr float s_font_size = 13.0f;
  static constexpr float console_font_size = 12.0f;

  // Icon sizes
  static constexpr float p_icon_size = 22.0f;
  static constexpr float p_bold_icon_size = 25.0f;
  static constexpr float h2_icon_size = 30.0f;
  static constexpr float h2_bold_icon_size = 30.0f;
  static constexpr float s_icon_size = 16.0f;
  static constexpr float s_bold_icon_size = 16.0f;

  // Layout
  static constexpr float window_padding = 30.0f;
  static constexpr float frame_padding = 3.0f;
};

#endif  // EDITOR_STYLES_H
