#include "editor_styles.h"

namespace EditorStyles {

static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

// Font internals
Fonts g_fonts;

// Access fonts
const Fonts& GetFonts() {
  return g_fonts;
}

void _MergeIcons(ImGuiIO& io, float icon_size) {
  float icons_font_size = icon_size * 2.0f / 3.0f;

  ImFontConfig icons_config;
  icons_config.MergeMode = true;
  icons_config.PixelSnapH = true;
  icons_config.GlyphMinAdvanceX = icons_font_size;

  g_fonts.icons = io.Fonts->AddFontFromFileTTF(
      EditorFontPath::icons, icons_font_size, &icons_config, icon_ranges);
}

void LoadFonts(ImGuiIO& io) {

  // Global font config
  ImFontConfig main_config;
  main_config.MergeMode = false;
  main_config.PixelSnapH = true;
  main_config.OversampleH = 3;
  main_config.OversampleV = 2;

  // p (paragraph)
  g_fonts.p = io.Fonts->AddFontFromFileTTF(
      EditorFontPath::regular, EditorSizes::p_font_size, &main_config);
  _MergeIcons(io, EditorSizes::p_icon_size);
  g_fonts.p_bold = io.Fonts->AddFontFromFileTTF(
      EditorFontPath::bold, EditorSizes::p_font_size, &main_config);
  _MergeIcons(io, EditorSizes::p_bold_icon_size);

  // h1
  g_fonts.h1 = io.Fonts->AddFontFromFileTTF(
      EditorFontPath::regular, EditorSizes::h1_font_size, &main_config);
  g_fonts.h1_bold = io.Fonts->AddFontFromFileTTF(
      EditorFontPath::bold, EditorSizes::h1_font_size, &main_config);

  // h2
  g_fonts.h2 = io.Fonts->AddFontFromFileTTF(
      EditorFontPath::regular, EditorSizes::h2_font_size, &main_config);
  _MergeIcons(io, EditorSizes::h2_icon_size);
  g_fonts.h2_bold = io.Fonts->AddFontFromFileTTF(
      EditorFontPath::bold, EditorSizes::h2_font_size, &main_config);
  _MergeIcons(io, EditorSizes::h2_bold_icon_size);

  // h3
  g_fonts.h3 = io.Fonts->AddFontFromFileTTF(
      EditorFontPath::regular, EditorSizes::h3_font_size, &main_config);
  _MergeIcons(io, EditorSizes::h2_icon_size);
  g_fonts.h3_bold = io.Fonts->AddFontFromFileTTF(
      EditorFontPath::bold, EditorSizes::h3_font_size, &main_config);
  _MergeIcons(io, EditorSizes::h2_icon_size);

  // h4
  g_fonts.h4 = io.Fonts->AddFontFromFileTTF(
      EditorFontPath::regular, EditorSizes::h4_font_size, &main_config);
  _MergeIcons(io, EditorSizes::p_bold_icon_size);
  g_fonts.h4_bold = io.Fonts->AddFontFromFileTTF(
      EditorFontPath::bold, EditorSizes::h4_font_size, &main_config);
  _MergeIcons(io, EditorSizes::p_bold_icon_size);

  // s (small)
  g_fonts.s = io.Fonts->AddFontFromFileTTF(
      EditorFontPath::regular, EditorSizes::s_font_size, &main_config);
  _MergeIcons(io, EditorSizes::s_icon_size);
  g_fonts.s_bold = io.Fonts->AddFontFromFileTTF(
      EditorFontPath::bold, EditorSizes::s_font_size, &main_config);
  _MergeIcons(io, EditorSizes::s_bold_icon_size);

  // console font config
  ImFontConfig console_config;
  console_config.OversampleH = 1;
  console_config.OversampleV = 1;
  console_config.PixelSnapH = true;
  console_config.RasterizerMultiply = 1.0f;

  // load font for console
  g_fonts.console = io.Fonts->AddFontFromFileTTF(
      EditorFontPath::console, EditorSizes::console_font_size, &console_config);
}

void SetupStyle() {
  ImGuiStyle& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;

  // === Core Layout (Deep Black Theme) ===
  colors[ImGuiCol_WindowBg] =
      ImVec4(0.08f, 0.08f, 0.08f, 1.00f);  // #141414 (Main Background)
  colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);    // #1A1A1A
  colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.96f);    // #1F1F1F
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);  // #141414

  // === Text & Icons ===
  colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f,
                                 1.00f);  // #E6E6E6 (Off-white for readability)
  colors[ImGuiCol_TextDisabled] =
      ImVec4(0.40f, 0.40f, 0.40f, 1.00f);  // #666666

  // === Borders (Lighter grey for high contrast against black) ===
  colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);  // #404040
  colors[ImGuiCol_BorderShadow] =
      ImVec4(0.00f, 0.00f, 0.00f, 0.00f);  // #00000000

  // === Arrows / Expanders / Headers ===
  colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);  // #2E2E2E
  colors[ImGuiCol_HeaderHovered] =
      ImVec4(0.25f, 0.25f, 0.25f, 1.00f);  // #404040
  colors[ImGuiCol_HeaderActive] =
      ImVec4(0.16f, 0.40f, 0.75f, 1.00f);  // #2966C0 (Vibrant Blue Accent)

  // === Frames / Inputs / Buttons ===
  colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);  // #262626
  colors[ImGuiCol_FrameBgHovered] =
      ImVec4(0.22f, 0.22f, 0.22f, 1.00f);  // #383838
  colors[ImGuiCol_FrameBgActive] =
      ImVec4(0.16f, 0.40f, 0.75f, 1.00f);                        // #2966C0
  colors[ImGuiCol_Button] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);  // #2E2E2E
  colors[ImGuiCol_ButtonHovered] =
      ImVec4(0.25f, 0.25f, 0.25f, 1.00f);  // #404040
  colors[ImGuiCol_ButtonActive] =
      ImVec4(0.16f, 0.40f, 0.75f, 1.00f);  // #2966C0

  // === Tabs ===
  colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);         // #1F1F1F
  colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);  // #404040
  colors[ImGuiCol_TabActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);   // #2E2E2E
  colors[ImGuiCol_TabUnfocused] =
      ImVec4(0.10f, 0.10f, 0.10f, 1.00f);  // #1A1A1A
  colors[ImGuiCol_TabUnfocusedActive] =
      ImVec4(0.15f, 0.15f, 0.15f, 1.00f);  // #262626
  style.Colors[ImGuiCol_TabSelectedOverline] =
      ImVec4(0.16f, 0.40f, 0.75f, 1.00f);  // #2966C0

  // === Title ===
  colors[ImGuiCol_TitleBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);  // #111111
  colors[ImGuiCol_TitleBgActive] =
      ImVec4(0.07f, 0.07f, 0.07f, 1.00f);  // #111111
  colors[ImGuiCol_TitleBgCollapsed] =
      ImVec4(0.07f, 0.07f, 0.07f, 1.00f);  // #111111

  // === Separator ===
  colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);  // #404040
  colors[ImGuiCol_SeparatorHovered] =
      ImVec4(0.16f, 0.40f, 0.75f, 1.00f);  // #2966C0
  colors[ImGuiCol_SeparatorActive] =
      ImVec4(0.16f, 0.40f, 0.75f, 1.00f);  // #2966C0

  // === Scroller ===
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);  // #1A1A1A
  colors[ImGuiCol_ScrollbarGrab] =
      ImVec4(0.25f, 0.25f, 0.25f, 1.00f);  // #404040
  colors[ImGuiCol_ScrollbarGrabHovered] =
      ImVec4(0.35f, 0.35f, 0.35f, 1.00f);  // #595959
  colors[ImGuiCol_ScrollbarGrabActive] =
      ImVec4(0.45f, 0.45f, 0.45f, 1.00f);  // #737373

  // === Plot Lines (Used for sliders/curves) ===
  colors[ImGuiCol_PlotLines] = ImVec4(0.16f, 0.40f, 0.75f, 1.00f);  // #2966C0

  // === Slider ===
  colors[ImGuiCol_SliderGrab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);  // #404040
  colors[ImGuiCol_SliderGrabActive] =
      ImVec4(0.16f, 0.40f, 0.75f, 1.00f);  // #2966C0

  // === Nav ===
  colors[ImGuiCol_NavWindowingHighlight] =
      ImVec4(0.16f, 0.40f, 0.75f, 1.00f);  // #2966C0
  colors[ImGuiCol_NavHighlight] =
      ImVec4(0.16f, 0.40f, 0.75f, 1.00f);  // #2966C0

  // === Grip ===
  colors[ImGuiCol_ResizeGrip] =
      ImVec4(0.15f, 0.15f, 0.15f, 0.00f);  // Transparent
  colors[ImGuiCol_ResizeGripHovered] =
      ImVec4(0.16f, 0.40f, 0.75f, 1.00f);  // #2966C0
  colors[ImGuiCol_ResizeGripActive] =
      ImVec4(0.16f, 0.40f, 0.75f, 1.00f);  // #2966C0

  // === Docking ===
  colors[ImGuiCol_DockingPreview] =
      ImVec4(0.16f, 0.40f, 0.75f, 0.60f);  // #2966C099
  colors[ImGuiCol_DockingEmptyBg] =
      ImVec4(0.08f, 0.08f, 0.08f, 1.00f);  // #141414

  // === Style Tuning ===
  style.FrameBorderSize = 1.0f;  // Essential for borders to show up
  style.WindowBorderSize = 1.0f;
  style.PopupBorderSize = 1.0f;
  style.TabBorderSize =
      1.0f;  // Giving tabs a subtle border outlines them against the black
  style.ChildBorderSize = 1.0f;  // Defines nested regions (Hierarchy)

  style.WindowRounding = 0.0f;
  style.FrameRounding = 3.0f;  // Keeps buttons friendly
  style.ScrollbarRounding = 0.0f;
  style.TabRounding = 2.0f;  // Slight rounding on tabs
  style.GrabRounding = 2.0f;

  // Padding Improvements
  style.WindowPadding = ImVec2(10.0f, 10.0f);  // Spacing from window edges
  style.FramePadding = ImVec2(
      16.0f,
      6.0f);  // (x:16, y:6) Makes buttons notably wider and taller around text
  style.CellPadding =
      ImVec2(8.0f, 4.0f);  // Protects text inside Inspector tables
  style.ItemSpacing = ImVec2(8.0f, 6.0f);  // Space between adjacent UI elements
  style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);

  style.GrabMinSize = 10.0f;
}

void Initialize() {
  ImGuiIO& io = ImGui::GetIO();
  LoadFonts(io);
  SetupStyle();

  io.FontGlobalScale = 0.9f;

#ifdef IMGUI_HAS_DOCK
  ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;

  ImGuiStyle& style = ImGui::GetStyle();
  style.TabRounding = 0.0f;
  style.TabBorderSize = 0.0f;
  style.TabBarOverlineSize =
      2.0f;  // Highlights the selected tab with the Unity Blue Accent
#endif
}

}  // namespace EditorStyles