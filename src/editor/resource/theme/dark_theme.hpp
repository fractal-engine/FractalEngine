#pragma once
#include "editor/vendor/IconFontCppHeaders/IconsFontAwesome6.h"  // FA mapping

#include "imgui.h"

namespace Theme {
inline ImFont* consoleFont = nullptr;  // Global font for console

inline void LoadFonts(ImGuiIO& io) {
  static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

  // main font config
  ImFontConfig main_font_config;
  main_font_config.MergeMode = false;
  main_font_config.PixelSnapH = true;
  main_font_config.SizePixels = 16.0f;      
  main_font_config.OversampleH = 1;    
  main_font_config.OversampleV = 1;

  io.Fonts->AddFontFromFileTTF("NotoSansMono_Regular.ttf", 16.0f, &main_font_config);

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
  consoleFont = io.Fonts->AddFontFromFileTTF("TerminusTTF-4.49.3.ttf", 12.0f,
                                             &console_config);
}

void ApplyStyle() {
  ImGuiStyle& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;

  // === Core Layout ===
  colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);  // #24292e
  colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);   // #24292e
  colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);   // #24292e

  // === Text & Icons ===
  colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
  // colors[ImGuiCol_Text] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);  // #b3b3b3

  // === Arrows / Expanders ===
  colors[ImGuiCol_Header] =
      ImVec4(0.12f, 0.13f, 0.14f, 0.00f);  // Transparent bg
  colors[ImGuiCol_HeaderHovered] =
      ImVec4(0.12f, 0.13f, 0.14f, 0.10f);  // light hover
  colors[ImGuiCol_HeaderActive] = ImVec4(0.12f, 0.13f, 0.14f, 0.15f);

  // === Frames / Inputs / Buttons ===
  colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.11f, 0.12f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.27f, 0.30f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4(0.18f, 0.20f, 0.22f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.28f, 0.30f, 0.32f, 1.00f);

  // === Tabs ===
  colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.14f, 0.16f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.16f, 0.18f, 0.20f, 1.00f);
  colors[ImGuiCol_TabActive] = ImVec4(0.12f, 0.14f, 0.16f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.14f, 0.16f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.14f, 0.16f, 1.00f);

  // === Misc ===
  colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.13f, 0.14f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.13f, 0.14f, 1.00f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.00f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.30f, 0.60f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.45f, 0.45f, 0.80f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
  colors[ImGuiCol_Border] =
      ImVec4(0.08f, 0.08f, 0.08f, 1.00f);  // inner borders
  colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

  // === Style Tuning ===
  style.FrameBorderSize = 0.0f;
  style.WindowBorderSize = 1.0f;
  style.PopupBorderSize = 0.0f;
  style.TabBorderSize = 0.0f;
  style.ChildBorderSize = 0.0f;

  style.WindowRounding = 4.0f;
  style.FrameRounding = 3.0f;
  style.ScrollbarRounding = 5.0f;
  style.GrabRounding = 3.0f;
  style.TabRounding = 3.0f;
}

inline void Initialize() {
  ImGuiIO& io = ImGui::GetIO();
  LoadFonts(io);
  ApplyStyle();
}
}  // namespace Theme
