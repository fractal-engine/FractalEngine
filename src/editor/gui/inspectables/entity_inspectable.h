#ifndef ENTITY_INSPECTABLE_H
#define ENTITY_INSPECTABLE_H

#include "inspectable_base.h"

#include "editor/gui/hierarchy_panel.h"
#include "engine/ecs/ecs_collection.h"

class EntityInspectable : public InspectableBase {
public:
  EntityInspectable(HierarchyItem& item);

  void RenderStaticContent(ImDrawList& draw_list) override;
  void RenderDynamicContent(ImDrawList& draw_list) override;

private:
  HierarchyItem item;
};

#endif  // ENTITY_INSPECTABLE_H