#ifndef EDITOR_LAYER_H
#define EDITOR_LAYER_H

#include <memory>
#include "editor/editor_base.h"
#include "imgui.h"
#include "renderer/renderer_graphics.h"
#include "subsystem/input/key_map_sdl.h"

class EditorLayer : public EditorBase {
public:
  explicit EditorLayer(std::unique_ptr<RendererBase>& renderer);
  ~EditorLayer();

  void Run() override;
  void RequestUpdate() override;
  void Shutdown() override;
  void BeginImGuiFrame(SDL_Window* window);

private:
  void HandleInput(Key key);
  void RenderUI();

  std::unique_ptr<RendererBase>& renderer_;
  bool quit_ = false;
  bool is_game_started_ = false;
  bool game_canvas_hovered_ = false;
};

#endif  // EDITOR_LAYER_H