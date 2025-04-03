#include "game/game_test.h"
#include <bgfx/bgfx.h>          // Fixed include path
#include "base/logger.h"        // Include Logger for logging
#include "base/shader_utils.h"  // Include ShaderUtils for shader loading
#include "subsystem/graphics_renderer.h"  // Include GraphicsRenderer for rendering
#include "subsystem/subsystem_manager.h"

#include <bx/math.h>

// Constructor: Initializes _helloWorldProgram to an invalid BGFX handle
GameTest::GameTest() : _helloWorldProgram(BGFX_INVALID_HANDLE) {}

// Destructor: Destroys the shader program if it is valid
GameTest::~GameTest() {
  if (bgfx::isValid(_helloWorldProgram))  // Check if the program is valid
    bgfx::destroy(_helloWorldProgram);    // Release the program resources
}

// Manages the shader program from the GraphicsRenderer
void GameTest::Init() {

  // Initialize the shader program
  GraphicsRenderer* renderer =
      static_cast<GraphicsRenderer*>(SubsystemManager::GetRenderer().get());

  const char* backend = bgfx::getRendererName(bgfx::getRendererType());
  std::string vsPath =
      std::string("assets/shaders/") + backend + "/vs_terrain.bin";
  std::string fsPath =
      std::string("assets/shaders/") + backend + "/fs_terrain.bin";

  _helloWorldProgram = renderer->LoadShaderProgram("terrain", vsPath, fsPath);

  // Example vertices
  struct PosColorVertex {
    float x, y;
    uint32_t abgr;
  };

  static PosColorVertex s_vertices[] = {
      {0.0f, 0.5f, 0xffff0000},   // top
      {0.5f, -0.5f, 0xff00ff00},  // right
      {-0.5f, -0.5f, 0xff0000ff}  // left
  };

  static const uint16_t s_indices[] = {0, 1, 2};

  bgfx::VertexLayout layout;
  layout.begin()
      .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
      .end();

  const bgfx::Memory* vtxMem = bgfx::copy(s_vertices, sizeof(s_vertices));
  vertexBuffer = bgfx::createVertexBuffer(vtxMem, layout);

  const bgfx::Memory* idxMem = bgfx::copy(s_indices, sizeof(s_indices));
  indexBuffer = bgfx::createIndexBuffer(idxMem);

  // Initialize world matrix to identity
  bx::mtxIdentity(world_matrix);
}

void GameTest::Update() {
  if (!bgfx::isValid(_helloWorldProgram))
    return;

  // BGFX draw calls
  // send draw commands to the renderer here
  bgfx::setTransform(world_matrix);
  bgfx::setState(BGFX_STATE_DEFAULT);
  bgfx::setVertexBuffer(0, vertexBuffer);
  bgfx::setIndexBuffer(indexBuffer);
  bgfx::submit(0, _helloWorldProgram);
}