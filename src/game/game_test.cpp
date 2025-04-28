#include <bgfx/bgfx.h>
#include <bx/math.h>

#include "game/game_test.h"

#include "core/logger.h"
#include "core/view_ids.h"
#include "renderer/shaders/shader_utils.h"

#include "renderer/renderer_graphics.h"

#include "subsystem/subsystem_manager.h"

bgfx::VertexLayout PosTexCoord0Vertex::layout;  // Definition

void PosTexCoord0Vertex::init() {
  layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .end();
}

// Constructor: Initializes _terrainProgram to an invalid BGFX handle
GameTest::GameTest()
    : _terrainProgramHeight(BGFX_INVALID_HANDLE),
      _heightUniform(BGFX_INVALID_HANDLE),
      _heightTexture(BGFX_INVALID_HANDLE),
      vertexBuffer(BGFX_INVALID_HANDLE),
      indexBuffer(BGFX_INVALID_HANDLE) {}

// Destructor: Destroys the shader program if it is valid
GameTest::~GameTest() {}

// Manages the shader program from the GraphicsRenderer
void GameTest::Init() {
  auto* renderer =
      static_cast<GraphicsRenderer*>(SubsystemManager::GetRenderer().get());

  auto& shaderMgr = *SubsystemManager::GetShaderManager();
  _terrainProgramHeight = shaderMgr.LoadProgram(
      "terrain_height", "vs_terrain_height_texture.bin", "fs_terrain.bin");

  _heightUniform =
      bgfx::createUniform("s_heightTexture", bgfx::UniformType::Sampler);
  Logger::getInstance().Log(LogLevel::Debug,
                            "[GameTest] Created height texture uniform");

  _lightDirUniform = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);

  PosTexCoord0Vertex::init();

  // Create height texture - grayscale
  const uint16_t size = 32;
  std::vector<uint8_t> heightData(size * size, 0);

  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      heightData[y * size + x] = static_cast<uint8_t>(
          127 + 127 * sinf(x * 0.2f));  // some wavy pattern
    }
  }

  const bgfx::Memory* mem = bgfx::copy(heightData.data(), heightData.size());
  _heightTexture = bgfx::createTexture2D(size, size, false, 1,
                                         bgfx::TextureFormat::R8, 0, mem);

  Logger::getInstance().Log(LogLevel::Debug,
                            "[GameTest] Created initial height texture");

  const uint16_t gridSize = 32;
  for (uint16_t y = 0; y < gridSize; ++y) {
    for (uint16_t x = 0; x < gridSize; ++x) {
      float xf = (float)x;
      float yf = (float)y;
      terrainVertices.push_back(
          {xf, 0.0f, yf, xf / (gridSize - 1), yf / (gridSize - 1)});
    }
  }

  for (uint16_t y = 0; y < gridSize - 1; ++y) {
    for (uint16_t x = 0; x < gridSize - 1; ++x) {
      uint16_t i = y * gridSize + x;
      terrainIndices.push_back(i);
      terrainIndices.push_back(i + 1);
      terrainIndices.push_back(i + gridSize);
      terrainIndices.push_back(i + 1);
      terrainIndices.push_back(i + gridSize + 1);
      terrainIndices.push_back(i + gridSize);
    }
  }

  const bgfx::Memory* vtxMem =
      bgfx::copy(terrainVertices.data(),
                 terrainVertices.size() * sizeof(PosTexCoord0Vertex));
  vertexBuffer = bgfx::createVertexBuffer(vtxMem, PosTexCoord0Vertex::layout);

  const bgfx::Memory* idxMem = bgfx::copy(
      terrainIndices.data(), terrainIndices.size() * sizeof(uint16_t));
  indexBuffer = bgfx::createIndexBuffer(idxMem);

  bx::mtxIdentity(world_matrix);
}

void GameTest::Update() {
  if (!bgfx::isValid(_terrainProgramHeight))
    return;

  // animate frame by frame
  static float t = 0.0f;
  t += 0.05f;

  const uint16_t size = 32;
  std::vector<uint8_t> heightData(size * size);

  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      heightData[y * size + x] =
          static_cast<uint8_t>(127 + 127 * sinf(x * 0.2f + t));
    }
  }

  const bgfx::Memory* mem = bgfx::copy(heightData.data(), heightData.size());
  bgfx::updateTexture2D(_heightTexture, 0, 0, 0, 0, size, size, mem);

  // physics, input, etc.
}
void SetLightDirectionWithIntensity(
    bgfx::UniformHandle lightUniform,  // Light intensity function
    float intensity, const float dir[3]) {
  if (!bgfx::isValid(lightUniform))
    return;

  // Normalize input direction
  float normDir[3];
  float len = bx::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
  normDir[0] = dir[0] / len;
  normDir[1] = dir[1] / len;
  normDir[2] = dir[2] / len;

  // Multiply normalized direction by intensity
  float lightDir[4] = {
      normDir[0] * intensity, normDir[1] * intensity, normDir[2] * intensity,
      0.0f  // directional light: w = 0
  };

  bgfx::setUniform(lightUniform, lightDir);
}
// TODO: lock or check thread safety here (in case games access state that's
// modified in game thread)
void GameTest::Render() {
  if (!bgfx::isValid(_terrainProgramHeight)) {
    Logger::getInstance().Log(LogLevel::Error,
                              "Terrain program invalid in Render()");
    return;
  }

  // Set view/projection matrices for SCENE view
  float view[16];
  float proj[16];

  // Setup camera matrices
  bx::Vec3 eye = bx::Vec3(cameraEye[0], cameraEye[1], cameraEye[2]);
  bx::Vec3 at = bx::Vec3(cameraAt[0], cameraAt[1], cameraAt[2]);
  bx::Vec3 up = bx::Vec3(cameraUp[0], cameraUp[1], cameraUp[2]);
  bx::mtxLookAt(view, eye, at, up);

  // Use canvas viewport dimensions for aspect ratio
  float aspect = (canvasViewportW > 0 && canvasViewportH > 0)
                     ? float(canvasViewportW) / float(canvasViewportH)
                     : 1.0f;

  bx::mtxProj(proj, cameraFOV, aspect, 0.1f, 1000.0f,
              bgfx::getCaps()->homogeneousDepth);

  // Scale terrain
  bx::mtxScale(world_matrix, 5.0f, 5.0f, 5.0f);

  // Explicit ViewID and transformations
  bgfx::setViewTransform(ViewID::SCENE, view, proj);
  bgfx::setTransform(world_matrix);

  // Set up buffers and textures
  bgfx::setVertexBuffer(0, vertexBuffer);
  bgfx::setIndexBuffer(indexBuffer);
  bgfx::setTexture(0, _heightUniform, _heightTexture);

  // Light settings
  const float direction[3] = {0.3f, 1.0f, 0.4f};
  SetLightDirectionWithIntensity(_lightDirUniform, 2.5f, direction);

  // Set render state with depth testing enabled
  bgfx::setState(BGFX_STATE_DEFAULT);

  // Submit to SCENE view, bound to framebuffer
  bgfx::submit(ViewID::SCENE, _terrainProgramHeight);

  // Debugging
  Logger::getInstance().Log(LogLevel::Debug,
                            "GameTest submitted render to ViewID::SCENE");
}

void GameTest::Shutdown() {
  Logger::getInstance().Log(LogLevel::Debug, "[GameTest] Shutdown()");

  if (bgfx::isValid(_terrainProgramHeight)) {
    Logger::getInstance().Log(LogLevel::Debug,
                              "[GameTest] Destroying terrain shader");
    bgfx::destroy(_terrainProgramHeight);
    _terrainProgramHeight = BGFX_INVALID_HANDLE;
  }

  if (bgfx::isValid(_heightTexture)) {
    Logger::getInstance().Log(LogLevel::Debug,
                              "[GameTest] Destroying height texture");
    bgfx::destroy(_heightTexture);
    _heightTexture = BGFX_INVALID_HANDLE;
  }

  if (bgfx::isValid(_heightUniform)) {
    Logger::getInstance().Log(LogLevel::Debug,
                              "[GameTest] Destroying height uniform");
    bgfx::destroy(_heightUniform);
    _heightUniform = BGFX_INVALID_HANDLE;
  }

  if (bgfx::isValid(_lightDirUniform)) {
    Logger::getInstance().Log(LogLevel::Debug,
                              "[GameTest] Destroying light uniform");
    bgfx::destroy(_lightDirUniform);
    _lightDirUniform = BGFX_INVALID_HANDLE;
  }

  if (bgfx::isValid(vertexBuffer)) {
    Logger::getInstance().Log(LogLevel::Debug,
                              "[GameTest] Destroying vertex buffer");
    bgfx::destroy(vertexBuffer);
    vertexBuffer = BGFX_INVALID_HANDLE;
  }

  if (bgfx::isValid(indexBuffer)) {
    Logger::getInstance().Log(LogLevel::Debug,
                              "[GameTest] Destroying index buffer");
    bgfx::destroy(indexBuffer);
    indexBuffer = BGFX_INVALID_HANDLE;
  }
}
