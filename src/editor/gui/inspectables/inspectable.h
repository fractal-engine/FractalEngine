#ifndef INSPECTABLE_H
#define INSPECTABLE_H

#include <imgui.h>
#include <string>

class Inspectable {

public:
  virtual void RenderStaticContent(ImDrawList& draw_list) = 0;
  virtual void RenderDynamicContent(ImDrawList& draw_list) = 0;
};

#endif  // INSPECTABLE_H