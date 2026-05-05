#include "asset_graph_editor.h"
#include <ImGuiFileDialog/ImGuiFileDialog.h>

#include <cmath>  // Needed for std::abs, std::round
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

// Set up json serializers so the nlohmann library knows how to save our custom
// structs
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
  j = nlohmann::json{{"part_id", p.part_id}};
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

// Utility function to convert strings to unique numerical ids for ImGui
static uintptr_t HashString(const std::string& str) {
  return std::hash<std::string>{}(str);
}

// Collapses raw lists of attachment points (e.g. 620 string entries) into a
// readable summary count
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

// Extracts the base category from a group ID (e.g., "_WINDOW_A" becomes
// "WINDOW") This allows a grouping of variants under a single consistent color
// scheme.
static std::string ExtractCategoryName(const std::string& group_id) {
  std::string category = group_id;

  // Strip leading underscores
  while (!category.empty() && category.front() == '_') {
    category.erase(0, 1);
  }

  // Keep everything up to the next underscore
  size_t pos = category.find('_');
  if (pos != std::string::npos) {
    category = category.substr(0, pos);
  }

  return category;
}

// Generates a consistent, dark-pastel color for the node header based on its
// category
static ImU32 GenerateGroupHeaderColor(const std::string& group_id) {
  std::string category = ExtractCategoryName(group_id);

  // Provide a neutral dark gray for the root base node
  if (category == "BASE") {
    return IM_COL32(70, 70, 70, 255);
  }

  // Hash the category string to generate a deterministic rgb color
  size_t hash = std::hash<std::string>{}(category);
  int r = (hash & 0xFF0000) >> 16;
  int g = (hash & 0x00FF00) >> 8;
  int b = (hash & 0x0000FF);

  // Mix the random color with a dark base to ensure it looks deep and
  // professional
  r = (r + 40) / 2;
  g = (g + 40) / 2;
  b = (b + 80) / 2;

  return IM_COL32(r, g, b, 255);
}

// Implementation of the editor window
AssetGraphEditor::AssetGraphEditor(PreviewData* data) : data_(data) {
  ed::Config config;
  config.SettingsFile = "AssetGraphEditor.json";
  m_EditorContext = ed::CreateEditor(&config);

  // Set Unreal Engine style dark background and grid colors
  ed::SetCurrentEditor(m_EditorContext);
  ed::Style& style = ed::GetStyle();
  style.Colors[ed::StyleColor_Bg] =
      ImVec4(0.08f, 0.08f, 0.08f, 1.0f);  // Deep dark grey/black
  style.Colors[ed::StyleColor_Grid] =
      ImVec4(0.16f, 0.16f, 0.16f, 1.0f);  // Subtle grid lines
  ed::SetCurrentEditor(nullptr);
}

AssetGraphEditor::~AssetGraphEditor() {
  ed::DestroyEditor(m_EditorContext);
}

void AssetGraphEditor::LoadGraph(const std::string& filepath) {
  m_ModelData = ProcModel::ModelDescriptor{};
  if (ProcModel::DescriptorParser::LoadFromFile(filepath, m_ModelData)) {
    m_CurrentFilePath = filepath;
    m_NeedsAutoLayout = true;
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

    // Trigger the preview window to regenerate models when saved
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

  // Render the top toolbar area
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
  ImGui::BeginChild("GraphToolbar", ImVec2(0, 36.0f), true,
                    ImGuiWindowFlags_NoScrollbar);

  if (ImGui::Button(ICON_FA_FOLDER_OPEN " Load")) {
    IGFD::FileDialogConfig cfg{};
    cfg.path = std::filesystem::current_path().string();
    IGFD::FileDialog::Instance()->OpenDialog("GraphLoadDlg",
                                             "Select Descriptor", ".json", cfg);
  }

  // Handle file dialog completion
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

  // Render the actual node graph workspace
  RenderNodeGraph();

  ImGui::End();
  ImGui::PopStyleVar();
}

// Automatically organizes nodes into a clean horizontal tree structure
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

  // Build the hierarchical tree based on 'activated_by' links
  for (auto& group : m_ModelData.selection_groups) {
    if (group.activated_by.empty()) {
      roots.push_back(&group);
    } else {
      std::string parentGroupId = partToGroup[group.activated_by];
      if (parentGroupId.empty()) {
        roots.push_back(&group);
      } else {
        tree[parentGroupId].push_back(&group);
      }
    }
  }

  std::unordered_map<std::string, float> nodeHeights;
  std::unordered_map<std::string, float> subtreeHeights;
  std::unordered_set<std::string> visited;

  // First pass: compute the total height required by each branch to avoid
  // overlapping
  std::function<float(ProcModel::SelectionGroup*)> computeHeight =
      [&](ProcModel::SelectionGroup* node) {
        if (visited.count(node->group_id))
          return 0.0f;
        visited.insert(node->group_id);

        float h = 100.0f + (node->parts.size() * 32.0f);
        if (!node->attach_to.empty())
          h += 35.0f;
        nodeHeights[node->group_id] = h;

        float childrenH = 0.0f;
        if (tree.count(node->group_id)) {
          for (auto* child : tree[node->group_id]) {
            childrenH += computeHeight(child) + 40.0f;
          }
          if (childrenH > 0)
            childrenH -= 40.0f;
        }
        subtreeHeights[node->group_id] = std::max(h, childrenH);
        return subtreeHeights[node->group_id];
      };

  for (auto* root : roots)
    computeHeight(root);

  // Second pass: physically assign positions based on the heights calculated
  visited.clear();
  std::function<void(ProcModel::SelectionGroup*, int, float)> placeNode =
      [&](ProcModel::SelectionGroup* node, int depth, float startY) {
        if (visited.count(node->group_id))
          return;
        visited.insert(node->group_id);

        ed::NodeId id = HashString(node->group_id);
        float x = depth * 420.0f;

        // Center the parent node vertically relative to its children
        float y = startY + (subtreeHeights[node->group_id] -
                            nodeHeights[node->group_id]) *
                               0.5f;
        ed::SetNodePosition(id, ImVec2(x, y));

        if (tree.count(node->group_id)) {
          float childY = startY;
          for (auto* child : tree[node->group_id]) {
            placeNode(child, depth + 1, childY);
            childY += subtreeHeights[child->group_id] + 40.0f;
          }
        }
      };

  float currentRootY = 0.0f;
  for (auto* root : roots) {
    placeNode(root, 0, currentRootY);
    currentRootY += subtreeHeights[root->group_id] + 80.0f;
  }
}

void AssetGraphEditor::RenderNodeGraph() {
  // Capture the editor's screen space coordinates before starting it
  ImVec2 editor_pos = ImGui::GetCursorScreenPos();
  ImVec2 editor_size = ImGui::GetContentRegionAvail();

  ed::SetCurrentEditor(m_EditorContext);
  ed::Begin("PCG_Node_Editor");

  bool trigger_nav = false;

  if (m_NeedsAutoLayout) {
    AutoLayoutNodes();
    trigger_nav = true;
    m_NeedsAutoLayout = false;
  }

  m_PinIdToString.clear();
  m_PinIdToGroup.clear();

  const float nodeWidth = 260.0f;

  for (auto& group : m_ModelData.selection_groups) {
    // Tighten node padding horizontally so pins can sit nicely on the outer
    // edges
    ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(4.0f, 8.0f, 4.0f, 8.0f));

    ed::NodeId nodeId = HashString(group.group_id);
    ed::BeginNode(nodeId);

    // Capture the exact top-left coordinate of the node's content area
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    // Calculate header dimensions safely
    float headerHeight = ImGui::GetTextLineHeight() + 12.0f;
    ImVec2 headerMin =
        ImVec2(cursorPos.x - 4.0f,
               cursorPos.y - 8.0f);  // account for the 4px padding we set above
    ImVec2 headerMax =
        ImVec2(headerMin.x + nodeWidth + 8.0f, headerMin.y + headerHeight);

    // Draw the colored header background
    ImGui::GetWindowDrawList()->AddRectFilled(
        headerMin, headerMax, GenerateGroupHeaderColor(group.group_id),
        ed::GetStyle().NodeRounding, ImDrawFlags_RoundCornersTop);

    // Position the text perfectly inside the drawn header box
    ImGui::SetCursorScreenPos(ImVec2(headerMin.x + 8.0f, headerMin.y + 6.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ImGui::TextUnformatted(group.group_id.c_str());
    if (group.required) {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "(Req)");
    }
    ImGui::PopStyleColor();

    // Push the cursor down beneath the header so the rest of the node content
    // renders correctly
    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, headerMax.y + 8.0f));

    // Force the node to expand to our desired fixed width
    ImGui::Dummy(ImVec2(nodeWidth, 0.0f));

    // Draw the activation input pin on the left side
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

    // Display attachment summaries if available
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

    // Draw a horizontal line separating the settings from the output styles
    ImGui::Dummy(ImVec2(0, 4));
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p0.x, p0.y),
                                        ImVec2(p0.x + nodeWidth, p0.y),
                                        IM_COL32(80, 80, 80, 255), 1.0f);
    ImGui::Dummy(ImVec2(0, 4));

    // Render individual parts as output pins
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

      // Align the output pin to the far right edge of the node
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

    // Button to append new parts to this group
    ImGui::Dummy(ImVec2(0, 5));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);

    ImGui::PushID(group.group_id.c_str());
    if (ImGui::Button("+ Add Part", ImVec2(nodeWidth - 16.0f, 0))) {
      ProcModel::PartDescriptor new_part;
      new_part.id = group.group_id + "_NEW_PART";
      new_part.name = "New Part";
      new_part.weight = 1.0f;
      group.parts.push_back(new_part);
    }
    ImGui::PopID();

    ed::EndNode();
    ed::PopStyleVar();
  }

  // Render all active connections between nodes
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

  // Process user interactions for creating new links
  if (ed::BeginCreate()) {
    ed::PinId inputPinId, outputPinId;
    if (ed::QueryNewLink(&inputPinId, &outputPinId)) {
      ProcModel::SelectionGroup* targetGroup = nullptr;
      std::string sourcePartId = "";

      // Ensure connection flows from an output pin to an input pin
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

  // Process user interactions for deleting existing links
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

  // Frame the camera gracefully after the layout algorithm runs
  if (trigger_nav) {
    ed::NavigateToContent();
  }

  ed::End();

  // Get the current zoom level from the node editor
  float zoom = ed::GetCurrentZoom();
  std::string zoom_text;
  if (std::abs(zoom - 1.0f) < 0.01f) {
    zoom_text = "Zoom 1:1";
  } else {
    zoom_text = "Zoom " +
                std::to_string(static_cast<int>(std::round(zoom * 100.0f))) +
                "%";
  }

  // Draw the text overlay floating in the top-right corner of the canvas view
  ImVec2 text_size = ImGui::CalcTextSize(zoom_text.c_str());
  ImVec2 text_pos = ImVec2(editor_pos.x + editor_size.x - text_size.x - 16.0f,
                           editor_pos.y + 16.0f);

  // Use GetWindowDrawList so the text sits permanently on top of the node grid
  ImGui::GetWindowDrawList()->AddText(text_pos, IM_COL32(180, 180, 180, 255),
                                      zoom_text.c_str());

  ed::SetCurrentEditor(nullptr);
}