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

  // === Core Layout ===
  colors[ImGuiCol_WindowBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);   // #383838
  colors[ImGuiCol_ChildBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);    // #383838
  colors[ImGuiCol_PopupBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);    // #282828
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);  // #282828

  // === Text & Icons ===
  colors[ImGuiCol_Text] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);  // #D2D2D2
  colors[ImGuiCol_TextDisabled] =
      ImVec4(0.53f, 0.53f, 0.53f, 1.00f);  // #888888

  // === Arrows / Expanders ===
  colors[ImGuiCol_Header] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);  // #454545
  colors[ImGuiCol_HeaderHovered] =
      ImVec4(0.35f, 0.35f, 0.35f, 1.00f);  // #595959
  colors[ImGuiCol_HeaderActive] =
      ImVec4(0.17f, 0.36f, 0.53f, 1.00f);  // #2C5D87 (Unity Accent Blue)

  // === Frames / Inputs / Buttons ===
  colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);  // #232323
  colors[ImGuiCol_FrameBgHovered] =
      ImVec4(0.18f, 0.18f, 0.18f, 1.00f);  // #2D2D2D
  colors[ImGuiCol_FrameBgActive] =
      ImVec4(0.17f, 0.36f, 0.53f, 1.00f);                        // #2C5D87
  colors[ImGuiCol_Button] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);  // #454545
  colors[ImGuiCol_ButtonHovered] =
      ImVec4(0.35f, 0.35f, 0.35f, 1.00f);  // #595959
  colors[ImGuiCol_ButtonActive] =
      ImVec4(0.17f, 0.36f, 0.53f, 1.00f);  // #2C5D87

  // === Tabs ===
  colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);         // #282828
  colors[ImGuiCol_TabHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);  // #454545
  colors[ImGuiCol_TabActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);   // #383838
  colors[ImGuiCol_TabUnfocused] =
      ImVec4(0.16f, 0.16f, 0.16f, 1.00f);  // #282828
  colors[ImGuiCol_TabUnfocusedActive] =
      ImVec4(0.22f, 0.22f, 0.22f, 1.00f);                             // #383838
  colors[ImGuiCol_TabSelected] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);  // #383838
  style.Colors[ImGuiCol_TabSelectedOverline] =
      ImVec4(0.17f, 0.36f, 0.53f, 1.00f);  // #2C5D87

  // === Title ===
  colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);  // #282828
  colors[ImGuiCol_TitleBgActive] =
      ImVec4(0.16f, 0.16f, 0.16f, 1.00f);  // #282828
  colors[ImGuiCol_TitleBgCollapsed] =
      ImVec4(0.16f, 0.16f, 0.16f, 1.00f);  // #282828

  // === Separator ===
  colors[ImGuiCol_Separator] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);  // #1A1A1A
  colors[ImGuiCol_SeparatorHovered] =
      ImVec4(0.17f, 0.36f, 0.53f, 1.00f);  // #2C5D87
  colors[ImGuiCol_SeparatorActive] =
      ImVec4(0.17f, 0.36f, 0.53f, 1.00f);  // #2C5D87

  // === Scroller ===
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);  // #232323
  colors[ImGuiCol_ScrollbarGrab] =
      ImVec4(0.27f, 0.27f, 0.27f, 1.00f);  // #454545
  colors[ImGuiCol_ScrollbarGrabHovered] =
      ImVec4(0.35f, 0.35f, 0.35f, 1.00f);  // #595959
  colors[ImGuiCol_ScrollbarGrabActive] =
      ImVec4(0.45f, 0.45f, 0.45f, 1.00f);  // #737373

  // === Border ===
  colors[ImGuiCol_Border] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);  // #1A1A1A
  colors[ImGuiCol_BorderShadow] =
      ImVec4(0.00f, 0.00f, 0.00f, 0.00f);                           // #00000000
  colors[ImGuiCol_PlotLines] = ImVec4(0.17f, 0.36f, 0.53f, 1.00f);  // #2C5D87

  // === Slider ===
  colors[ImGuiCol_SliderGrab] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);  // #454545
  colors[ImGuiCol_SliderGrabActive] =
      ImVec4(0.17f, 0.36f, 0.53f, 1.00f);  // #2C5D87

  // === Nav ===
  colors[ImGuiCol_NavWindowingHighlight] =
      ImVec4(0.17f, 0.36f, 0.53f, 1.00f);  // #2C5D87
  colors[ImGuiCol_NavHighlight] =
      ImVec4(0.17f, 0.36f, 0.53f, 1.00f);  // #2C5D87

  // === Grip ===
  colors[ImGuiCol_ResizeGrip] =
      ImVec4(0.14f, 0.14f, 0.14f, 0.00f);  // #23232300
  colors[ImGuiCol_ResizeGripHovered] =
      ImVec4(0.17f, 0.36f, 0.53f, 1.00f);  // #2C5D87
  colors[ImGuiCol_ResizeGripActive] =
      ImVec4(0.17f, 0.36f, 0.53f, 1.00f);  // #2C5D87

  // === Docking ===
  colors[ImGuiCol_DockingPreview] =
      ImVec4(0.17f, 0.36f, 0.53f, 0.60f);  // #2C5D8799
  colors[ImGuiCol_DockingEmptyBg] =
      ImVec4(0.22f, 0.22f, 0.22f, 1.00f);  // #383838

  // === Style Tuning ===
  style.FrameBorderSize = 1.0f;
  style.WindowBorderSize = 1.0f;
  style.PopupBorderSize = 1.0f;
  style.TabBorderSize = 0.0f;
  style.ChildBorderSize = 0.0f;

  style.WindowRounding = 0.0f;
  style.FrameRounding = 3.0f;  // Bumping to 3.0f makes buttons look slightly
                               // softer and friendlier (for Celestine)
  style.ScrollbarRounding = 0.0f;
  style.TabRounding = 0.0f;
  style.GrabRounding = 2.0f;

  // Adjusting padding makes the UI look less cramped
  style.WindowPadding =
      ImVec2(8.0f, 8.0f);  // Adds breathing room to panel edges
  style.FramePadding = ImVec2(
      12.0f, 5.0f);  // (x:12, y:5) Makes buttons wider and slightly taller
  style.ItemSpacing =
      ImVec2(8.0f, 6.0f);  // Increases vertical space between items in lists
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