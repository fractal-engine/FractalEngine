#ifndef ASSET_GRAPH_EDITOR_H
#define ASSET_GRAPH_EDITOR_H

#include <imgui.h>

class AssetGraphEditor {
public:
  AssetGraphEditor();
  void Render();

private:
  void RenderNodeGraph(ImDrawList* draw_list);
};

#endif  // ASSET_GRAPH_EDITOR_H