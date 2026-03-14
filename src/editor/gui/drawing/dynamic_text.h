#ifndef DYNAMIC_TEXT_H
#define DYNAMIC_TEXT_H

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <cstdint>
#include <string>

#include "draw_alignments.h"

class DynamicText {
public:
  DynamicText(ImFont& font);

  // PROPERTIES

  // Pointer to font to use
  ImFont& font;

  // Text content
  std::string text;

  // Text color
  ImU32 color = IM_COL32(0, 0, 0, 255);

  // Positioning of text
  ImVec2 position = ImVec2(0.0f, 0.0f);

  // Horzontal/vertical padding around text
  ImVec2 padding = ImVec2(0.0f, 0.0f);

  // Text alignment within parent component
  TextAlign alignment = TextAlign::LEFT;

  // Draw UI text
  void Draw(ImDrawList* draw_list);

  // Rweturn absolute size of text in px
  ImVec2 GetSize() const;
};

#endif  // DYNAMIC_TEXT_H