#include "dynamic_text.h"

DynamicText::DynamicText(ImFont& font) : font(font) {}

void DynamicText::Draw(ImDrawList* draw_list) {

  // Calculate draw position
  ImVec2 draw_pos = position + padding;

  // Add to draw list
  draw_list->AddText(&font, font.FontSize, draw_pos, color, text.c_str());
}

ImVec2 DynamicText::GetSize() const {

  // Get base size from displayed text
  ImVec2 size = font.CalcTextSizeA(font.FontSize, FLT_MAX, -1.0f, text.c_str());

  // Return total size with padding
  return size + padding * 2;
}
