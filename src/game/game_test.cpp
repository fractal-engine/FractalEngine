#include <bgfx/bgfx.h>
#include <bx/math.h>

#include "game/game_test.h"

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

// helper
static void initLayouts() {
  PosTexCoord0Vertex::layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .end();

  PosVertex::layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
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
      _sunVbh(BGFX_INVALID_HANDLE),
      _sunIbh(BGFX_INVALID_HANDLE),
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
  createSunBuffers();
}

// ──────────────────────────────────────────────────────
//  helper – generate cube for sky
// ──────────────────────────────────────────────────────
void GameTest::createSkyboxBuffers() {
  // 8 cube vertices – 36 indices (12 tris)
  static constexpr float v[] = {
      -1, -1, -1, 1, -1, -1, 1, 1, -1, -1, 1, -1,  // back
      -1, -1, 1,  1, -1, 1,  1, 1, 1,  -1, 1, 1    // front
  };
  static constexpr uint16_t i[] = {
      0, 1, 2, 0, 2, 3,  // back
      4, 6, 5, 4, 7, 6,  // front
      4, 5, 1, 4, 1, 0,  // bottom
      3, 2, 6, 3, 6, 7,  // top
      1, 5, 6, 1, 6, 2,  // right
      4, 0, 3, 4, 3, 7   // left
  };
  _skyVbh =
      bgfx::createVertexBuffer(bgfx::makeRef(v, sizeof(v)), PosVertex::layout);
  _skyIbh = bgfx::createIndexBuffer(bgfx::makeRef(i, sizeof(i)));
}

// sun is just a quad billboard
void GameTest::createSunBuffers() {
  static constexpr float v[] = {-10, -10, 0, 10, -10, 0, 10, 10, 0, -10, 10, 0};
  static constexpr uint16_t i[] = {0, 1, 2, 2, 3, 0};

  _sunVbh =
      bgfx::createVertexBuffer(bgfx::makeRef(v, sizeof(v)), PosVertex::layout);
  _sunIbh = bgfx::createIndexBuffer(bgfx::makeRef(i, sizeof(i)));
}

// ──────────────────────────────────────────────────────
//  Update()
// ──────────────────────────────────────────────────────
void GameTest::Update() {
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

  // camera matrices
  float view[16], proj[16];
  bx::mtxLookAt(view, bx::Vec3{cameraEye[0], cameraEye[1], cameraEye[2]},
                bx::Vec3{cameraAt[0], cameraAt[1], cameraAt[2]},
                bx::Vec3{cameraUp[0], cameraUp[1], cameraUp[2]});

  const float aspect = (canvasViewportW > 0 && canvasViewportH > 0)
                           ? float(canvasViewportW) / float(canvasViewportH)
                           : 1.f;
  bx::mtxProj(proj, cameraFOV, aspect, 0.1f, 1000.f,
              bgfx::getCaps()->homogeneousDepth);

  // ---- View IDs ----
  constexpr uint8_t kSceneView = ViewID::SCENE;     // 1
  constexpr uint8_t kSkyView = ViewID::SCENE_N(0);  // 2 – shares FBO

  // ---- sky-box ----
  if (bgfx::isValid(_skyProgram)) {
    // --- view matrix: camera rotation, no translation -------------
    float skyView[16];
    bx::memCopy(skyView, view, sizeof(skyView));
    skyView[12] = skyView[13] = skyView[14] = 0.0f;  // centre on eye
    bgfx::setViewTransform(kSkyView, skyView, proj);

    // --- model matrix: looks like huge cube at the origin ---------
    float skyMtx[16];
    bx::mtxScale(skyMtx, 500.0f, 500.0f, 500.0f);  // 500-unit box
    bgfx::setTransform(skyMtx);

    bgfx::setVertexBuffer(0, _skyVbh);
    bgfx::setIndexBuffer(_skyIbh);

    // uniforms -----------------------------------------------------
    bx::Vec3 sunDir = bx::normalize(
        bx::Vec3{cosf(_cycleTime), sinf(_cycleTime), sinf(_cycleTime * 0.5f)});

    float time[4] = {_cycleTime, 0, 0, 0};
    float dir[4] = {sunDir.x, sunDir.y, sunDir.z, 0};

    bgfx::setUniform(_timeUniform, time);
    bgfx::setUniform(_sunDirUniform, dir);
    bgfx::setUniform(_sunLumUniform, _sunColorArray);
    bgfx::setUniform(_paramsUniform, _parametersArray);

    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_DEPTH_TEST_ALWAYS);  // depth read yes, write no
    bgfx::submit(kSkyView, _skyProgram);
  }

  // ---- terrain ----
  bx::mtxScale(world_matrix, 5.f, 5.f, 5.f);  // scale terrain
  bgfx::setViewTransform(kSceneView, view, proj);
  bgfx::setTransform(world_matrix);
  bgfx::setVertexBuffer(0, _terrainVbh);
  bgfx::setIndexBuffer(_terrainIbh);
  bgfx::setTexture(0, _heightUniform, _heightTexture);

  const float lightDir3[3] = {0.3f, 1.f, 0.4f};
  float tmp[4] = {lightDir3[0] * 2.5f, lightDir3[1] * 2.5f, lightDir3[2] * 2.5f,
                  0};
  bgfx::setUniform(_lightDirUniform, tmp);

  bgfx::setState(BGFX_STATE_DEFAULT);
  bgfx::submit(kSceneView, _terrainProgramHeight);

  // ---- sun billboard ----
  if (bgfx::isValid(_sunProgram)) {
    bx::Vec3 sunDir = bx::normalize(
        bx::Vec3{cosf(_cycleTime), sinf(_cycleTime), sinf(_cycleTime * 0.5f)});
    bx::Vec3 sunPos = bx::mul(sunDir, 100.f);

    float sunMtx[16];
    bx::mtxTranslate(sunMtx, sunPos.x, sunPos.y, sunPos.z);
    bgfx::setTransform(sunMtx);
    bgfx::setViewTransform(kSceneView, view, proj);

    bgfx::setVertexBuffer(0, _sunVbh);
    bgfx::setIndexBuffer(_sunIbh);

    float dir[4] = {sunDir.x, sunDir.y, sunDir.z, 0};
    float time[4] = {_cycleTime, 0, 0, 0};
    _parametersArray[3] = _cycleTime;

    bgfx::setUniform(_sunDirUniform, dir);
    bgfx::setUniform(_sunLumUniform, _sunColorArray);
    bgfx::setUniform(_paramsUniform, _parametersArray);
    bgfx::setUniform(_timeUniform, time);

    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_BLEND_ADD |
                   BGFX_STATE_DEPTH_TEST_LESS);

    bgfx::submit(kSceneView, _sunProgram);
  }
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

  destroy(_terrainVbh);
  destroy(_terrainIbh);
  destroy(_skyVbh);
  destroy(_skyIbh);
  destroy(_sunVbh);
  destroy(_sunIbh);
}
