#pragma once
#include "editor/vendor/IconFontCppHeaders/IconsFontAwesome6.h"  // FA mapping

#include "imgui.h"

namespace Theme {
inline ImFont* console_font = nullptr;  // Global font for console

inline void LoadFonts(ImGuiIO& io) {
  static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

  // main font config
  ImFontConfig main_font_config;
  main_font_config.MergeMode = false;
  main_font_config.PixelSnapH = true;
  main_font_config.SizePixels = 16.0f;
  main_font_config.OversampleH = 3;
  main_font_config.OversampleV = 2;

  io.Fonts->AddFontFromFileTTF("NotoSansMono_Regular.ttf", 16.0f,
                               &main_font_config);

  // icons font config
  ImFontConfig icons_config;
  icons_config.MergeMode = true;
  icons_config.PixelSnapH = true;
  icons_config.GlyphMinAdvanceX = 16.0f;

  io.Fonts->AddFontFromFileTTF("fa-solid-900.ttf", 16.0f, &icons_config,
                               icon_ranges);

  // console font config
  ImFontConfig console_config;
  console_config.SizePixels = 12.0f;
  console_config.OversampleH = 1;
  console_config.OversampleV = 1;
  console_config.PixelSnapH = true;
  console_config.RasterizerMultiply = 1.0f;

  // load font for console
  console_font = io.Fonts->AddFontFromFileTTF("TerminusTTF-4.49.3.ttf", 12.0f,
                                              &console_config);
}

void ApplyStyle() {
  ImGuiStyle& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;

  // === Core Layout ===
  colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);    // #24292e
  colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);     // #24292e
  colors[ImGuiCol_PopupBg] = ImVec4(0.098f, 0.102f, 0.110f, 0.95f);  // #191a1c
  colors[ImGuiCol_MenuBarBg] =
      ImVec4(0.098f, 0.102f, 0.110f, 1.00f);  // #191a1c

  // === Text & Icons ===
  colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);  // #E6E6E6
  colors[ImGuiCol_TextDisabled] =
      ImVec4(0.50f, 0.50f, 0.50f, 1.00f);  // #808080
  // colors[ImGuiCol_Text] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);  // #b3b3b3

  // === Arrows / Expanders ===
  colors[ImGuiCol_Header] = ImVec4(0.12f, 0.13f, 0.14f, 0.00f);  // #1F212300
  colors[ImGuiCol_HeaderHovered] =
      ImVec4(0.12f, 0.13f, 0.14f, 0.10f);  //  #1F212319
  colors[ImGuiCol_HeaderActive] =
      ImVec4(0.12f, 0.13f, 0.14f, 0.15f);  // #1F212326

  // === Frames / Inputs / Buttons ===
  colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.13f, 0.14f, 1.00f);  // #1F2123FF
  colors[ImGuiCol_FrameBgHovered] =
      ImVec4(0.20f, 0.22f, 0.25f, 1.00f);  // #33393FFF
  colors[ImGuiCol_FrameBgActive] =
      ImVec4(0.25f, 0.27f, 0.30f, 1.00f);                        // #40454DFF
  colors[ImGuiCol_Button] = ImVec4(0.18f, 0.20f, 0.22f, 1.00f);  // #2E3338FF
  colors[ImGuiCol_ButtonHovered] =
      ImVec4(0.24f, 0.26f, 0.28f, 1.00f);  // #3D4247FF
  colors[ImGuiCol_ButtonActive] =
      ImVec4(0.28f, 0.30f, 0.32f, 1.00f);  // #474C52FF

  // === Tabs ===
  colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);  // #25292e
  colors[ImGuiCol_TabHovered] =
      ImVec4(0.16f, 0.18f, 0.20f, 1.00f);                           // #292E33FF
  colors[ImGuiCol_TabActive] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);  // #25292e
  colors[ImGuiCol_TabUnfocused] =
      ImVec4(0.14f, 0.16f, 0.18f, 1.00f);  // #25292e
  colors[ImGuiCol_TabUnfocusedActive] =
      ImVec4(0.14f, 0.16f, 0.18f, 1.00f);  // #25292e
  // colors[ImGuiCol_TabSelected] = ImVec4(0.251f, 0.541f, 0.894f, 1.00f);  //
  // #408AE4

  // === Title ===
  colors[ImGuiCol_TitleBg] = ImVec4(0.098f, 0.102f, 0.110f, 1.00f);  // #191a1c
  colors[ImGuiCol_TitleBgActive] =
      ImVec4(0.098f, 0.102f, 0.110f, 1.00f);  // #191a1c
  colors[ImGuiCol_TitleBgCollapsed] =
      ImVec4(0.098f, 0.102f, 0.110f, 1.00f);  // #191a1c

  // === Separator ===
  colors[ImGuiCol_Separator] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

  // === Scroller ===
  colors[ImGuiCol_ScrollbarBg] =
      ImVec4(0.10f, 0.10f, 0.10f, 0.00f);  // #19191900
  colors[ImGuiCol_ScrollbarGrab] =
      ImVec4(0.30f, 0.30f, 0.30f, 0.60f);  // #4C4C4C99
  colors[ImGuiCol_ScrollbarGrabHovered] =
      ImVec4(0.45f, 0.45f, 0.45f, 0.80f);  // #737373CC
  colors[ImGuiCol_ScrollbarGrabActive] =
      ImVec4(0.60f, 0.60f, 0.60f, 1.00f);  // #999999FF

  // === Border ===
  colors[ImGuiCol_Border] = ImVec4(0.098f, 0.102f, 0.110f, 1.00f);  // #191a1c
  colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);               // #00000000
  colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);  // blue

  // === Slider ===
  colors[ImGuiCol_SliderGrab] =
      ImVec4(0.251f, 0.541f, 0.894f, 1.00f);  // #408AE4

  // === Nav ===
  colors[ImGuiCol_NavWindowingHighlight] =
      ImVec4(0.251f, 0.541f, 0.894f, 1.00f);  // #408AE4
  colors[ImGuiCol_NavHighlight] = ImVec4(0.25f, 0.75f, 0.35f, 1.00f);

  // === Table ===
  // colors[ImGuiCol_TableBorderStrong] = ImVec4(0.27f, 0.23f, 0.29f, 1.00f);

  // === Grip ===
  colors[ImGuiCol_ResizeGripHovered] =
      ImVec4(0.251f, 0.541f, 0.894f, 1.00f);  // #408AE4
  colors[ImGuiCol_ResizeGripActive] =
      ImVec4(0.251f, 0.541f, 0.894f, 1.00f);  // #408AE4

  // === Docking ===
  colors[ImGuiCol_DockingPreview] =
      ImVec4(0.251f, 0.541f, 0.894f, 0.60f);  // rgba(64, 138, 228, 0.6)
  colors[ImGuiCol_DockingEmptyBg] =
      ImVec4(0.14f, 0.16f, 0.18f, 1.00f);  // Match background

  // === Style Tuning ===
  style.FrameBorderSize = 0.0f;
  style.WindowBorderSize = 1.0f;
  style.PopupBorderSize = 1.0f;
  style.TabBorderSize = 0.0f;
  style.ChildBorderSize = 0.0f;
  style.WindowRounding = 4.0f;
  style.FrameRounding = 3.0f;
  style.FramePadding.x = 15.0f;
  style.FramePadding.y = 4.0f;  // Vertical padding inside menu items
  style.ItemSpacing.x = 7.0f;
  style.ItemSpacing.y = 4.0f;
  style.ScrollbarRounding = 4.0f;
  style.GrabRounding = 2.0f;
  style.GrabMinSize = 7.5f;
  style.TabRounding = 4.0f;
}

#ifdef IMGUI_HAS_DOCK

#endif

inline void Initialize() {
  ImGuiIO& io = ImGui::GetIO();
  LoadFonts(io);
  ApplyStyle();
}
}  // namespace Theme
