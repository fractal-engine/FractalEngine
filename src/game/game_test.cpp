#include <bgfx/bgfx.h>
#include <bx/math.h>

#include "game/game_test.h"

#include <SDL.h>
#include "core/logger.h"
#include "core/view_ids.h"
#include "renderer/renderer_graphics.h"
#include "renderer/shaders/shader_utils.h"
#include "subsystem/subsystem_manager.h"

// ──────────────────────────────────────────────────────
//  Vertex layouts
// ──────────────────────────────────────────────────────
bgfx::VertexLayout PosTexCoord0Vertex::layout;

// sky-box & sun use position-only verts
struct PosVertex {
  float x, y, z;
  static bgfx::VertexLayout layout;
};
bgfx::VertexLayout PosVertex::layout;

struct ScreenPosVertex {
  float x, y;
  static bgfx::VertexLayout layout;
};
bgfx::VertexLayout ScreenPosVertex::layout;

// ──────────────────────────────────────────────────────
// helper to initialize vertex layouts
static void initLayouts() {
  PosTexCoord0Vertex::layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .end();

  PosVertex::layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .end();

  ScreenPosVertex::layout.begin()
      .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
      .end();
}

// ──────────────────────────────────────────────────────
//  GameTest ctor / dtor
// ──────────────────────────────────────────────────────
GameTest::GameTest()
    : _terrainProgramHeight(BGFX_INVALID_HANDLE),
      _heightUniform(BGFX_INVALID_HANDLE),
      _heightTexture(BGFX_INVALID_HANDLE),
      _lightDirUniform(BGFX_INVALID_HANDLE),
      _terrainVbh(BGFX_INVALID_HANDLE),
      _terrainIbh(BGFX_INVALID_HANDLE),
      _skyProgram(BGFX_INVALID_HANDLE),
      _sunProgram(BGFX_INVALID_HANDLE),
      _skyVbh(BGFX_INVALID_HANDLE),
      _skyIbh(BGFX_INVALID_HANDLE),
      _timeUniform(BGFX_INVALID_HANDLE),
      _sunDirUniform(BGFX_INVALID_HANDLE),
      _sunLumUniform(BGFX_INVALID_HANDLE),
      _paramsUniform(BGFX_INVALID_HANDLE),
      _cycleTime(0.f) {
  bx::mtxIdentity(world_matrix);
}

GameTest::~GameTest() = default;

// ──────────────────────────────────────────────────────
//  Init()
// ──────────────────────────────────────────────────────
void GameTest::Init() {
  initLayouts();

  auto& shaderMgr = *SubsystemManager::GetShaderManager();

  // ― Terrain shader
  _terrainProgramHeight = SubsystemManager::GetShaderManager()->LoadProgram(
      "terrain_height", "vs_terrain_height_texture.bin", "fs_terrain.bin");

  // ― Sky / Sun
  _skyProgram =
      shaderMgr.LoadProgram("skybox", "vs_skybox.bin", "fs_skybox.bin");
  _sunProgram = shaderMgr.LoadProgram("sun", "vs_sun.bin", "fs_sun.bin");

  // ― Uniforms
  _heightUniform =
      bgfx::createUniform("s_heightTexture", bgfx::UniformType::Sampler);
  _lightDirUniform = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);

  _timeUniform = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
  _sunDirUniform =
      bgfx::createUniform("u_sunDirection", bgfx::UniformType::Vec4);
  _sunLumUniform =
      bgfx::createUniform("u_sunLuminance", bgfx::UniformType::Vec4);
  _paramsUniform = bgfx::createUniform("u_parameters", bgfx::UniformType::Vec4);
  _viewInvUniform = bgfx::createUniform("u_viewInv", bgfx::UniformType::Mat4);
  _projInvUniform = bgfx::createUniform("u_projInv", bgfx::UniformType::Mat4);


  // height map (should be 32×32 sine-wave so we can still see animation)
  const uint16_t sz = 32;
  std::vector<uint8_t> hdata(sz * sz);
  for (int y = 0; y < sz; ++y)
    for (int x = 0; x < sz; ++x)
      hdata[y * sz + x] = uint8_t(127 + 127 * sinf(x * 0.2f));

  _heightTexture =
      bgfx::createTexture2D(sz, sz, false, 1, bgfx::TextureFormat::R8, 0,
                            bgfx::copy(hdata.data(), hdata.size()));

  // terrain mesh
  const uint16_t grid = 32;
  for (uint16_t y = 0; y < grid; ++y)
    for (uint16_t x = 0; x < grid; ++x)
      terrainVertices.push_back({float(x), 0.f, float(y), float(x) / (grid - 1),
                                 float(y) / (grid - 1)});

  for (uint16_t y = 0; y < grid - 1; ++y)
    for (uint16_t x = 0; x < grid - 1; ++x) {
      uint16_t i = y * grid + x;
      terrainIndices.push_back(i);
      terrainIndices.push_back(i + 1);
      terrainIndices.push_back(i + grid);
      terrainIndices.push_back(i + 1);
      terrainIndices.push_back(i + grid + 1);
      terrainIndices.push_back(i + grid);
    }

  _terrainVbh = bgfx::createVertexBuffer(
      bgfx::copy(terrainVertices.data(),
                 terrainVertices.size() * sizeof(PosTexCoord0Vertex)),
      PosTexCoord0Vertex::layout);
  _terrainIbh = bgfx::createIndexBuffer(bgfx::copy(
      terrainIndices.data(), terrainIndices.size() * sizeof(uint16_t)));

  createSkyboxBuffers();
}

// ──────────────────────────────────────────────────────
//  helper – generate sky buffers
// ──────────────────────────────────────────────────────
void GameTest::createSkyboxBuffers() {
  // Fullscreen quad in NDC space (x, y in [-1, 1])
  static const ScreenPosVertex quadVertices[] = {
      {-1.0f, -1.0f},
      {1.0f, -1.0f},
      {-1.0f, 1.0f},
      {1.0f, 1.0f},
  };

  static const uint16_t quadIndices[] = {
      0, 1, 2, 1, 3, 2,
  };

  _skyVbh = bgfx::createVertexBuffer(
      bgfx::makeRef(quadVertices, sizeof(quadVertices)),
      ScreenPosVertex::layout);

  _skyIbh =
      bgfx::createIndexBuffer(bgfx::makeRef(quadIndices, sizeof(quadIndices)));
}


// ──────────────────────────────────────────────────────
//  Update()
// ──────────────────────────────────────────────────────
void GameTest::Update() {

  // --- Camera Input Handling ---
  const Uint8* keys = SDL_GetKeyboardState(nullptr);

  if (keys[SDL_SCANCODE_W])
    camera.pan(0.0f, 0.10f);
  if (keys[SDL_SCANCODE_S])
    camera.pan(0.0f, -0.10f);
  if (keys[SDL_SCANCODE_A])
    camera.pan(-0.10f, 0.0f);
  if (keys[SDL_SCANCODE_D])
    camera.pan(0.10f, 0.0f);

  /* -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - */
  if (!bgfx::isValid(_terrainProgramHeight))
    return;

  // animate height map & day/night timer
  _cycleTime += 0.01f;

  const uint16_t sz = 32;
  std::vector<uint8_t> h(sz * sz);
  for (int y = 0; y < sz; ++y)
    for (int x = 0; x < sz; ++x)
      h[y * sz + x] = uint8_t(127 + 127 * sinf(x * 0.2f + _cycleTime));

  bgfx::updateTexture2D(_heightTexture, 0, 0, 0, 0, sz, sz,
                        bgfx::copy(h.data(), h.size()));
}

// ──────────────────────────────────────────────────────
//  Render()
// ──────────────────────────────────────────────────────
void GameTest::Render() {
  if (!bgfx::isValid(_terrainProgramHeight))
    return;

  // --- Camera view and projection ---
  float view[16], proj[16];
  camera.getViewMatrix(view);
  float aspect = canvasViewportW > 0 && canvasViewportH > 0
                     ? float(canvasViewportW) / canvasViewportH
                     : 1.0f;
  camera.getProjectionMatrix(proj, aspect, 60.0f);

  constexpr uint8_t kSceneView = ViewID::SCENE;
  constexpr uint8_t kSkyView = ViewID::SCENE_N(0);  // shared FBO

  // --- SKYBOX (procedural full-screen quad) ---
  if (bgfx::isValid(_skyProgram)) {
    float invView[16], invProj[16];
    bx::mtxInverse(invView, view);
    bx::mtxInverse(invProj, proj);

    // View & proj are identity for screen-space quad
    float identity[16];
    bx::mtxIdentity(identity);
    bgfx::setViewTransform(kSkyView, identity, identity);

    // Submit fullscreen quad 
    bgfx::setVertexBuffer(0, _skyVbh);
    bgfx::setIndexBuffer(_skyIbh);

    // Sun direction (cycle-based rotation)
    bx::Vec3 sunDir = bx::normalize(
        bx::Vec3{cosf(_cycleTime), sinf(_cycleTime), sinf(_cycleTime * 0.5f)});

    float time[4] = {_cycleTime, 0, 0, 0};
    float dir[4] = {sunDir.x, sunDir.y, sunDir.z, 0};

    float t = (sinf(_cycleTime) + 1.0f) * 0.5f;
    _sunColorArray[0] = bx::lerp(1.5f, 5.0f, t);
    _sunColorArray[1] = bx::lerp(0.8f, 5.0f, t);
    _sunColorArray[2] = bx::lerp(0.2f, 5.0f, t);
    _sunColorArray[3] = 0.0f;

    _parametersArray[0] = 0.1f;        // Sun size
    _parametersArray[1] = 1.0f;        // Bloom factor
    _parametersArray[2] = 1.0f;        // Exposure (unused)
    _parametersArray[3] = _cycleTime;  // Time

    bgfx::setUniform(_viewInvUniform, invView);
    bgfx::setUniform(_projInvUniform, invProj);
    bgfx::setUniform(_timeUniform, time);
    bgfx::setUniform(_sunDirUniform, dir);
    bgfx::setUniform(_sunLumUniform, _sunColorArray);
    bgfx::setUniform(_paramsUniform, _parametersArray);

    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_DEPTH_TEST_ALWAYS | BGFX_STATE_CULL_CW);

    bgfx::submit(kSkyView, _skyProgram);
  }


  // --- TERRAIN ---
  bx::mtxScale(world_matrix, 5.f, 5.f, 5.f);
  bgfx::setViewTransform(kSceneView, view, proj);
  bgfx::setTransform(world_matrix);
  bgfx::setVertexBuffer(0, _terrainVbh);
  bgfx::setIndexBuffer(_terrainIbh);
  bgfx::setTexture(0, _heightUniform, _heightTexture);

  const float lightDir3[3] = {0.3f, 1.f, 0.4f};
  float tmp[4] = {lightDir3[0] * 2.5f, lightDir3[1] * 2.5f, lightDir3[2] * 2.5f,
                  0};
  bgfx::setUniform(_lightDirUniform, tmp);

  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                 BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS |
                 BGFX_STATE_MSAA

  );

  bgfx::submit(kSceneView, _terrainProgramHeight);
}

// ──────────────────────────────────────────────────────
//  Shutdown()
// ──────────────────────────────────────────────────────
void GameTest::Shutdown() {
  auto destroy = [](auto& h) {
    if (bgfx::isValid(h)) {
      bgfx::destroy(h);
      h = BGFX_INVALID_HANDLE;
    }
  };

  destroy(_terrainProgramHeight);
  destroy(_skyProgram);
  destroy(_sunProgram);

  destroy(_heightTexture);
  destroy(_heightUniform);
  destroy(_lightDirUniform);
  destroy(_timeUniform);
  destroy(_sunDirUniform);
  destroy(_sunLumUniform);
  destroy(_paramsUniform);
  destroy(_viewInvUniform);
  destroy(_projInvUniform);
  destroy(_terrainVbh);
  destroy(_terrainIbh);
  destroy(_skyVbh);
  destroy(_skyIbh);
}
