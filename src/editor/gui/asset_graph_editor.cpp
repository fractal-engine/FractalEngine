#include "asset_graph_editor.h"
#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <functional>
#include <unordered_set>  // Needed for cycle protection
#include "engine/context/engine_context.h"
#include "engine/core/logger.h"
#include "engine/pcg/pcg_engine.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <map>

#ifndef ICON_FA_PLUS
#define ICON_FA_PLUS "+"
#define ICON_FA_FLOPPY_DISK "Save"
#define ICON_FA_FOLDER_OPEN "Load"
#define ICON_FA_LINK "@"
#endif

namespace ed = ax::NodeEditor;

// -------------------------------
// JSON Serializer
// -------------------------------
namespace glm {
void to_json(nlohmann::json& j, const vec3& v) {
  j = nlohmann::json::array({v.x, v.y, v.z});
}
}  // namespace glm

NLOHMANN_JSON_SERIALIZE_ENUM(
    ProcModel::ConstraintRule::Type,
    {{ProcModel::ConstraintRule::Type::EXCLUDES, "excludes"},
     {ProcModel::ConstraintRule::Type::REQUIRES, "requires"}})

namespace ProcModel {
void to_json(nlohmann::json& j, const PartDescriptor& p) {
  j = nlohmann::json{{"id", p.id}, {"name", p.name}, {"weight", p.weight}};
}
void to_json(nlohmann::json& j, const ConstraintRule& c) {
  j = nlohmann::json{{"id", c.id},
                     {"type", c.type},
                     {"part_a", c.part_a},
                     {"part_b", c.part_b}};
}
void to_json(nlohmann::json& j, const ParameterBinding& p) {
  j = nlohmann::json{{"source_part", p.source_part},
                     {"source_param", p.source_param},
                     {"target_part", p.target_part},
                     {"target_param", p.target_param},
                     {"ratio", p.ratio}};
}
void to_json(nlohmann::json& j, const SelectionGroup& g) {
  j = nlohmann::json{{"group_id", g.group_id},
                     {"required", g.required},
                     {"activated_by", g.activated_by},
                     {"parts", g.parts}};
  if (!g.attach_to.empty())
    j["attach_to"] = g.attach_to;
}
void to_json(nlohmann::json& j, const ParameterRange& p) {
  j = nlohmann::json{{"part_id", p.part_id}, {"activated_by", p.activated_by}};
  if (p.scale_min)
    j["scale_min"] = *p.scale_min;
  if (p.scale_max)
    j["scale_max"] = *p.scale_max;
  if (p.rotation_min)
    j["rotation_min"] = *p.rotation_min;
  if (p.rotation_max)
    j["rotation_max"] = *p.rotation_max;
}
void to_json(nlohmann::json& j, const ModelDescriptor& m) {
  j = nlohmann::json{{"model_id", m.model_id},
                     {"model_name", m.model_name},
                     {"path", m.path},
                     {"domain", m.domain},
                     {"selection_groups", m.selection_groups},
                     {"parameter_ranges", m.parameter_ranges},
                     {"constraints", m.constraints},
                     {"parameter_bindings", m.parameter_bindings}};
  if (m.scale_min)
    j["scale_min"] = *m.scale_min;
  if (m.scale_max)
    j["scale_max"] = *m.scale_max;
  if (!m.tags.empty())
    j["tags"] = m.tags;
}
}  // namespace ProcModel

// -------------------------------
// Helper Functions
// -------------------------------
static uintptr_t HashString(const std::string& str) {
  return std::hash<std::string>{}(str);
}

static std::map<std::string, int> GroupAttachmentPoints(
    const std::vector<std::string>& attachments) {
  std::map<std::string, int> grouped;
  for (const auto& att : attachments) {
    std::string base_name = att;
    while (!base_name.empty() && std::isdigit(base_name.back()))
      base_name.pop_back();
    while (!base_name.empty() &&
           (base_name.back() == '_' || base_name.back() == '.'))
      base_name.pop_back();
    grouped[base_name]++;
  }
  return grouped;
}

// -------------------------------
// Editor Implementation
// -------------------------------

AssetGraphEditor::AssetGraphEditor(PreviewData* data) : data_(data) {
  ed::Config config;
  config.SettingsFile = "AssetGraphEditor.json";
  m_EditorContext = ed::CreateEditor(&config);
}

AssetGraphEditor::~AssetGraphEditor() {
  ed::DestroyEditor(m_EditorContext);
}

void AssetGraphEditor::LoadGraph(const std::string& filepath) {
  m_ModelData = ProcModel::ModelDescriptor{};
  if (ProcModel::DescriptorParser::LoadFromFile(filepath, m_ModelData)) {
    m_CurrentFilePath = filepath;
    m_NeedsAutoLayout = true;  // Trigger layout on next frame
    Logger::getInstance().Log(LogLevel::Info,
                              "[AssetGraphEditor] Loaded: " + filepath);
  }
}

void AssetGraphEditor::SaveGraph(const std::string& filepath) {
  if (filepath.empty())
    return;

  nlohmann::json j = m_ModelData;
  std::ofstream file(filepath);
  if (file.is_open()) {
    file << j.dump(2);
    Logger::getInstance().Log(LogLevel::Info,
                              "[AssetGraphEditor] Saved: " + filepath);

    // Sync with the engine
    if (data_) {
      auto& pcg = EngineContext::Generator();
      data_->archetype_id = pcg.LoadArchetype(filepath);
      data_->instances.clear();
    }
  } else {
    Logger::getInstance().Log(LogLevel::Error,
                              "[AssetGraphEditor] Failed to save: " + filepath);
  }
}

void AssetGraphEditor::Render() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("Asset Graph", nullptr);

  // --- Toolbar ---
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
  ImGui::BeginChild("GraphToolbar", ImVec2(0, 36.0f), true,
                    ImGuiWindowFlags_NoScrollbar);

  if (ImGui::Button(ICON_FA_FOLDER_OPEN " Load")) {
    IGFD::FileDialogConfig cfg{};
    cfg.path = std::filesystem::current_path().string();
    IGFD::FileDialog::Instance()->OpenDialog("GraphLoadDlg",
                                             "Select Descriptor", ".json", cfg);
  }

  if (IGFD::FileDialog::Instance()->Display("GraphLoadDlg",
                                            ImGuiWindowFlags_NoCollapse,
                                            ImVec2(700.0f, 500.0f))) {
    if (IGFD::FileDialog::Instance()->IsOk()) {
      LoadGraph(IGFD::FileDialog::Instance()->GetFilePathName());
    }
    IGFD::FileDialog::Instance()->Close();
  }

  ImGui::SameLine();

  if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save")) {
    SaveGraph(m_CurrentFilePath);
  }

  ImGui::SameLine();
  ImGui::TextDisabled(" | ");
  ImGui::SameLine();

  if (ImGui::Button(ICON_FA_PLUS " Add Node")) {
    ProcModel::SelectionGroup new_group;
    new_group.group_id =
        "NEW_GROUP_" + std::to_string(m_ModelData.selection_groups.size());
    m_ModelData.selection_groups.push_back(new_group);
  }

  ImGui::SameLine();
  std::string display_name =
      m_CurrentFilePath.empty() ? "Unsaved File" : m_ModelData.model_name;
  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "  Editing: %s",
                     display_name.c_str());

  ImGui::EndChild();
  ImGui::PopStyleVar();

  // --- Node Editor Implementation ---
  RenderNodeGraph();

  ImGui::End();
  ImGui::PopStyleVar();
}

void AssetGraphEditor::AutoLayoutNodes() {
  if (m_ModelData.selection_groups.empty())
    return;

  std::unordered_map<std::string, std::string> partToGroup;
  for (const auto& group : m_ModelData.selection_groups) {
    for (const auto& part : group.parts) {
      partToGroup[part.id] = group.group_id;
    }
  }

  std::unordered_map<std::string, std::vector<ProcModel::SelectionGroup*>> tree;
  std::vector<ProcModel::SelectionGroup*> roots;

  for (auto& group : m_ModelData.selection_groups) {
    if (group.activated_by.empty()) {
      roots.push_back(&group);
    } else {
      std::string parentGroupId = partToGroup[group.activated_by];
      if (parentGroupId.empty()) {
        roots.push_back(&group);  // Fallback if link is broken
      } else {
        tree[parentGroupId].push_back(&group);
      }
    }
  }

  float currentY = 0.0f;
  std::unordered_set<std::string> visited;  // Cycle protection

  std::function<void(ProcModel::SelectionGroup*, int)> placeNode =
      [&](ProcModel::SelectionGroup* node, int depth) {
        // Prevent infinite loops if artist creates a circular reference
        if (visited.count(node->group_id))
          return;
        visited.insert(node->group_id);

        ed::NodeId id = HashString(node->group_id);
        float x = depth * 350.0f;
        float y = currentY;

        ed::SetNodePosition(id, ImVec2(x, y));

        float estimatedHeight = 110.0f + (node->parts.size() * 35.0f);
        if (!node->attach_to.empty())
          estimatedHeight += 40.0f;

        currentY += estimatedHeight;

        if (tree.count(node->group_id)) {
          for (auto* child : tree[node->group_id]) {
            placeNode(child, depth + 1);
          }
        }
      };

  for (auto* root : roots) {
    placeNode(root, 0);
    currentY += 80.0f;
  }
}

void AssetGraphEditor::RenderNodeGraph() {
  ed::SetCurrentEditor(m_EditorContext);
  ed::Begin("PCG_Node_Editor");

  bool trigger_nav = false;

  if (m_NeedsAutoLayout) {
    AutoLayoutNodes();
    trigger_nav = true;  // Tell it to frame the nodes at the end of the loop
    m_NeedsAutoLayout = false;
  }

  m_PinIdToString.clear();
  m_PinIdToGroup.clear();

  const float nodeWidth = 260.0f;

  for (auto& group : m_ModelData.selection_groups) {
    ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(4.0f, 8.0f, 4.0f, 8.0f));

    ed::NodeId nodeId = HashString(group.group_id);
    ed::BeginNode(nodeId);

    // 1. HEADER
    ImGui::Dummy(ImVec2(nodeWidth, 0));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
    ImGui::TextUnformatted(group.group_id.c_str());
    if (group.required) {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "(Req)");
    }
    ImGui::Dummy(ImVec2(0, 5));

    // 2. INPUT PIN (Left edge)
    ed::PinId inputPinId = HashString(group.group_id + "_IN");
    m_PinIdToGroup[inputPinId.Get()] = &group;

    ed::BeginPin(inputPinId, ed::PinKind::Input);
    ImVec2 posIn = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(12, 12));
    ImGui::GetWindowDrawList()->AddCircleFilled(
        ImVec2(posIn.x + 6, posIn.y + 6), 5.0f, IM_COL32(220, 180, 50, 255));
    ImGui::GetWindowDrawList()->AddCircle(ImVec2(posIn.x + 6, posIn.y + 6),
                                          5.0f, IM_COL32(30, 30, 30, 255), 12,
                                          1.5f);
    ed::EndPin();

    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);
    ImGui::Text("Activate");

    // 3. ATTACHMENTS
    if (!group.attach_to.empty()) {
      ImGui::Dummy(ImVec2(0, 4));
      auto grouped_attachments = GroupAttachmentPoints(group.attach_to);

      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
      ImGui::Text("Attachments:");
      for (const auto& [base_name, count] : grouped_attachments) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
        ImGui::Text(" %s %s (%d)", ICON_FA_LINK, base_name.c_str(), count);
      }
      ImGui::PopStyleColor();
      ImGui::Dummy(ImVec2(0, 4));
    }

    // 4. CUSTOM SEPARATOR
    ImGui::Dummy(ImVec2(0, 4));
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p0.x, p0.y),
                                        ImVec2(p0.x + nodeWidth, p0.y),
                                        IM_COL32(80, 80, 80, 255), 1.0f);
    ImGui::Dummy(ImVec2(0, 4));

    // 5. OUTPUT PINS (Right edge)
    for (auto& part : group.parts) {
      ed::PinId outputPinId = HashString(part.id);
      m_PinIdToString[outputPinId.Get()] = part.id;

      ImGui::PushID(part.id.c_str());

      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
      ImGui::PushItemWidth(90.0f);
      ImGui::SliderFloat("##w", &part.weight, 0.0f, 5.0f, "W: %.1f");
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::TextUnformatted(part.name.c_str());

      // Pin alignment
      ImGui::SameLine(nodeWidth - 12.0f);
      ed::BeginPin(outputPinId, ed::PinKind::Output);
      ImVec2 posOut = ImGui::GetCursorScreenPos();
      ImGui::Dummy(ImVec2(12, 12));
      ImGui::GetWindowDrawList()->AddCircleFilled(
          ImVec2(posOut.x + 6, posOut.y + 6), 5.0f,
          IM_COL32(100, 180, 220, 255));
      ImGui::GetWindowDrawList()->AddCircle(ImVec2(posOut.x + 6, posOut.y + 6),
                                            5.0f, IM_COL32(30, 30, 30, 255), 12,
                                            1.5f);
      ed::EndPin();

      ImGui::PopID();
    }

    // 6. ADD STYLE BUTTON
    ImGui::Dummy(ImVec2(0, 5));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);

    // Give the button a unique ID context
    ImGui::PushID(group.group_id.c_str());

    if (ImGui::Button("+ Add Style", ImVec2(nodeWidth - 16.0f, 0))) {
      ProcModel::PartDescriptor new_part;
      new_part.id = group.group_id + "_NEW_STYLE";
      new_part.name = "New Style";
      new_part.weight = 1.0f;
      group.parts.push_back(new_part);
    }

    ImGui::PopID();  

    ed::EndNode();
    ed::PopStyleVar();
  } 

  // Draw Link cables
  int link_id_counter = 1;
  for (const auto& group : m_ModelData.selection_groups) {
    if (!group.activated_by.empty()) {
      ed::LinkId linkId = link_id_counter++;
      ed::PinId outputPinId = HashString(group.activated_by);
      ed::PinId inputPinId = HashString(group.group_id + "_IN");

      ed::Link(linkId, outputPinId, inputPinId, ImVec4(0.3f, 0.7f, 0.9f, 1.0f),
               2.0f);
    }
  }

  // Handle Cable dragging
  if (ed::BeginCreate()) {
    ed::PinId inputPinId, outputPinId;
    if (ed::QueryNewLink(&inputPinId, &outputPinId)) {
      ProcModel::SelectionGroup* targetGroup = nullptr;
      std::string sourcePartId = "";

      if (m_PinIdToGroup.count(inputPinId.Get()) &&
          m_PinIdToString.count(outputPinId.Get())) {
        targetGroup = m_PinIdToGroup[inputPinId.Get()];
        sourcePartId = m_PinIdToString[outputPinId.Get()];
      } else if (m_PinIdToGroup.count(outputPinId.Get()) &&
                 m_PinIdToString.count(inputPinId.Get())) {
        targetGroup = m_PinIdToGroup[outputPinId.Get()];
        sourcePartId = m_PinIdToString[inputPinId.Get()];
      }

      if (targetGroup && !sourcePartId.empty()) {
        if (ed::AcceptNewItem(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), 2.0f)) {
          targetGroup->activated_by = sourcePartId;
        }
      } else {
        ed::RejectNewItem(ImVec4(1, 0, 0, 1), 2.0f);
      }
    }
  }
  ed::EndCreate();

  // Handle deleting link cables (ALT+Click or Delete key)
  if (ed::BeginDelete()) {
    ed::LinkId deletedLinkId;
    if (ed::QueryDeletedLink(&deletedLinkId)) {
      if (ed::AcceptDeletedItem()) {
        int link_idx = 1;
        for (auto& group : m_ModelData.selection_groups) {
          if (!group.activated_by.empty()) {
            if (link_idx == deletedLinkId.Get()) {
              group.activated_by = "";
              break;
            }
            link_idx++;
          }
        }
      }
    }
  }
  ed::EndDelete();

  // --- Navigate to content AFTER nodes are fully drawn ---
  if (trigger_nav) {
    ed::NavigateToContent();
  }

  ed::End();
  ed::SetCurrentEditor(nullptr);
}