#ifndef ENTITY_INSPECTABLE_H
#define ENTITY_INSPECTABLE_H

#include "engine/ecs/ecs_collection.h"
#include "inspectable_base.h"

class EntityInspectable : public InspectableBase {
public:
  EntityInspectable(Entity entity);

  void RenderStaticContent(ImDrawList& draw_list) override;
  void RenderDynamicContent(ImDrawList& draw_list) override;

  Entity GetEntity() const { return entity_; }

private:
  Entity entity_;
};

#endif  // ENTITY_INSPECTABLE_H