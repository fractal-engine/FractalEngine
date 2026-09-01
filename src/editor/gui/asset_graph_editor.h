#ifndef ASSET_GRAPH_EDITOR_H
#define ASSET_GRAPH_EDITOR_H

#include <imgui.h>
#include <imgui_node_editor.h>
#include <string>
#include <unordered_map>

#include "engine/pcg/procmodel/descriptor/model_descriptor.h"
#include "engine/pcg/procmodel/descriptor/model_descriptor_parser.h"

// Includes needed for engine syncing
#include "editor/gui/window_base.h"
#include "model_preview.h"  // To access PreviewData

class AssetGraphEditor : public WindowBase {
public:
  // Take PreviewData so we can sync the engine when we save
  AssetGraphEditor(PreviewData* data);
  ~AssetGraphEditor() override;

  void Render() override;

private:
  void RenderNodeGraph();
  void LoadGraph(const std::string& filepath);
  void SaveGraph(const std::string& filepath);

  void AutoLayoutNodes();
  bool m_NeedsAutoLayout = false;

  // Safe JSON mapping for Drag & Drop Interactions
  std::unordered_map<uintptr_t, std::string>
      m_OutputPinToPartId;  // Output Pin -> Part String ID
  std::unordered_map<uintptr_t, int>
      m_InputPinToGroupIdx;  // Input Pin -> Target Group Index
  std::unordered_map<uintptr_t, std::pair<int, std::string>>
      m_LinkIdToData;  // Link ID -> {TargetIdx, SourcePart}

  ax::NodeEditor::EditorContext* m_EditorContext = nullptr;
  PreviewData* data_ = nullptr;

  // Core Data
  ProcModel::ModelDescriptor m_ModelData;
  std::string m_CurrentFilePath = "";

  // Reverse lookups for ImGui Graph interaction
  std::unordered_map<uintptr_t, std::string> m_PinIdToString;
  std::unordered_map<uintptr_t, ProcModel::SelectionGroup*> m_PinIdToGroup;
};

#endif  // ASSET_GRAPH_EDITOR_H