#ifndef HIERARCHY_PANEL_H
#define HIERARCHY_PANEL_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "editor/gui/window_base.h"

#include "editor/gui/styles/editor_styles.h"
#include "engine/ecs/ecs_collection.h"

//---------------------------------------------------------------------------
// TODO:
// - search/filter function
// - rename context menu implementation
// - camera movement integration
// - rename EditorColor to actual theme usage
// - replace direct imgui code for drawing with dynamic drawing impl
// - fg
//---------------------------------------------------------------------------
struct HierarchyItem {
  explicit HierarchyItem(EntityContainer entity,
                         std::vector<HierarchyItem> children = {})
      : entity(entity), children(children), expanded(false) {};

  EntityContainer entity;
  std::vector<HierarchyItem> children;
  bool expanded;

  bool operator==(const HierarchyItem& other) const {
    return entity.Id() == other.entity.Id();
  }
};

//---------------------------------------------------------------------------
// HIERARCHY PANEL
// - Includes scene graph view with tree, drag-drop reparenting, multi-selection
//---------------------------------------------------------------------------
class HierarchyPanel : public WindowBase {
public:
  HierarchyPanel();

  void Render() override;
  void Invalidate();

private:
  // Rendering
  void RenderSearch(ImDrawList& draw_list);
  void RenderHierarchy(ImDrawList& draw_list);
  void RenderItem(ImDrawList& draw_list, HierarchyItem& item,
                  uint32_t indentation);
  void RenderDraggedItem();
  void RenderPopupMenu();

  // Hierarchy building
  void BuildSceneHierarchy();
  void BuildHierarchyChildren(HierarchyItem& parent);

  // Camera movement to entity
  void SetCameraTarget(TransformComponent* target);
  void UpdateCameraMovement();

  // Auto-scroll when dragging
  void PerformAutoScroll();

  void OnEntityChanged(entt::registry&, entt::entity);

  // Data
  char search_buffer[256];
  bool popup_menu_used;

  std::vector<HierarchyItem> current_hierarchy;
  std::unordered_map<uint32_t, HierarchyItem*> selected_items;
  HierarchyItem* last_selected;
  HierarchyItem* last_hovered;

  // Drag state
  bool dragging_hierarchy;

  // Camera movement state
  bool camera_moving;
  float camera_movement_time;
  TransformComponent* camera_target;

  // Handle cache
  bool hierarchy_dirty_ = true;

  // EnTT handles
  entt::scoped_connection on_create_connection_;
  entt::scoped_connection on_destroy_connection_;
};

#endif  // HIERARCHY_PANEL_H