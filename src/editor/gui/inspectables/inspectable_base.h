#ifndef INSPECTABLE_BASE_H
#define INSPECTABLE_BASE_H

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

class InspectableBase {

public:
  virtual ~InspectableBase() = default;
  virtual void RenderStaticContent(ImDrawList& draw_list) = 0;
  virtual void RenderDynamicContent(ImDrawList& draw_list) = 0;
};

#endif  // INSPECTABLE_BASE_H