#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include <entt/entt.hpp>
#include "events.h"

using Entity = entt::entity;

struct EditorState {
  // Selection
  Entity selected_entity = entt::null;
  Entity last_selected_entity = entt::null;

  void SelectEntity(Entity entity) {
    if (entity == entt::null)
      return;
    selected_entity = entity;
    last_selected_entity = entity;
    EditorEvents::entity_selected(entity);
  }

  void ClearSelection() {
    selected_entity = entt::null;
    EditorEvents::selection_cleared();
  }
};

#endif  // EDITOR_STATE_H