#include <bgfx/bgfx.h>
#include <bx/math.h>

#include "game/game_test.h"

#include <SDL.h>
#include "core/logger.h"
#include "core/view_ids.h"
#include "lighting/sky_lighting.h"
#include "renderer/renderer_graphics.h"
#include "renderer/shaders/shader_utils.h"
#include "subsystem/subsystem_manager.h"
#include "tools/texture_utils.h"

constexpr uint8_t shadowView = ViewID::SHADOW_PASS;
constexpr float TERRAIN_GLOBAL_SCALE = 5.0f;

// smoothStep function
inline float smoothStep(float edge0, float edge1, float x) {
  float t = bx::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

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

struct ShadowVertex {
  float x, y, z;
  float u, v;
  static bgfx::VertexLayout layout;
};

bgfx::VertexLayout ShadowVertex::layout;

SkyLighting skyLighting;  // Declare the SkyLighting instance

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

  ShadowVertex::layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .end();
}

// ──────────────────────────────────────────────────────
//  GameTest ctor / dtor
// ──────────────────────────────────────────────────────
GameTest::GameTest()
    : camera(),
      cameraSystem(&camera),
      _terrainProgramHeight(BGFX_INVALID_HANDLE),
      _heightUniform(BGFX_INVALID_HANDLE),
      _heightTexture(BGFX_INVALID_HANDLE),
      _lightDirUniform(BGFX_INVALID_HANDLE),
      _terrainVbh(BGFX_INVALID_HANDLE),
      _terrainIbh(BGFX_INVALID_HANDLE),
      _lightMatrixUniform(BGFX_INVALID_HANDLE),
      _skyAmbientUniform(BGFX_INVALID_HANDLE),
      _shadowSamplerUniform(BGFX_INVALID_HANDLE),
      _skyProgram(BGFX_INVALID_HANDLE),
      _skyVbh(BGFX_INVALID_HANDLE),
      _skyIbh(BGFX_INVALID_HANDLE),
      _timeUniform(BGFX_INVALID_HANDLE),
      _sunDirUniform(BGFX_INVALID_HANDLE),
      _sunLumUniform(BGFX_INVALID_HANDLE),
      _paramsUniform(BGFX_INVALID_HANDLE),
      _terrainShadowProgram(BGFX_INVALID_HANDLE),
      _s_diffuseUniform(BGFX_INVALID_HANDLE),
      _s_ormUniform(BGFX_INVALID_HANDLE),
      _s_normalUniform(BGFX_INVALID_HANDLE),
      _cycleTime(0.f) {
  bx::mtxIdentity(world_matrix);
}

GameTest::~GameTest() = default;

// ──────────────────────────────────────────────────────
//  Init()
// ──────────────────────────────────────────────────────
void GameTest::Init() {

  const bgfx::Caps* caps = bgfx::getCaps();

  std::string msg = "[BGFX] Max texture samplers supported: " +
                    std::to_string(caps->limits.maxTextureSamplers);
  Logger::getInstance().Log(LogLevel::Debug, msg);

  initLayouts();
  skyLighting.Init();  // Start the sky lighting system
  Logger::getInstance().Log(LogLevel::Debug, "[GameTest] Init() called.");

  // We have class-defined TerrainExtent
  float terrainCenter[3] = {TerrainExtent, 0.0f, TerrainExtent};

  camera.setTarget(terrainCenter);
  camera.setPitch(0.4f);
  camera.setYaw(0.75f);

  auto& shaderMgr = *SubsystemManager::GetShaderManager();

  // ― Terrain shader
  _terrainProgramHeight = SubsystemManager::GetShaderManager()->LoadProgram(
      "terrain_height", "vs_terrain.bin", "fs_terrain.bin");

  // ― Sky / Sun
  _skyProgram =
      shaderMgr.LoadProgram("skybox", "vs_skybox.bin", "fs_skybox.bin");

  // ― Shadow-only terrain shader
  _terrainShadowProgram = SubsystemManager::GetShaderManager()->LoadProgram(
      "terrain_shadow", "vs_shadow.bin", "fs_shadow.bin");

  // ― Uniforms
  _heightUniform =
      bgfx::createUniform("s_heightTexture", bgfx::UniformType::Sampler);
  _lightDirUniform = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);

  _timeUniform = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
  _cameraPosUniform =
      bgfx::createUniform("u_cameraPos", bgfx::UniformType::Vec4);
  _sunDirUniform =
      bgfx::createUniform("u_sunDirection", bgfx::UniformType::Vec4);
  _sunLumUniform =
      bgfx::createUniform("u_sunLuminance", bgfx::UniformType::Vec4);
  _paramsUniform = bgfx::createUniform("u_parameters", bgfx::UniformType::Vec4);
  _viewInvUniform = bgfx::createUniform("u_viewInv", bgfx::UniformType::Mat4);
  _projInvUniform = bgfx::createUniform("u_projInv", bgfx::UniformType::Mat4);

  _s_diffuseUniform =
      bgfx::createUniform("s_diffuse", bgfx::UniformType::Sampler);
  _s_ormUniform = bgfx::createUniform("s_orm", bgfx::UniformType::Sampler);
  _s_normalUniform =
      bgfx::createUniform("s_normal", bgfx::UniformType::Sampler);

  // Sky ambient light Uniform
  _skyAmbientUniform =
      bgfx::createUniform("u_skyAmbient", bgfx::UniformType::Vec4);
  // Light Matrix Uniform
  _lightMatrixUniform =
      bgfx::createUniform("u_lightMatrix", bgfx::UniformType::Mat4);

  _shadowSamplerUniform =
      bgfx::createUniform("s_shadowMap", bgfx::UniformType::Sampler);

  // Load terrain textures
  terrainDiffuse =
      TextureUtils::LoadTexture("assets/textures/terrain/basecolor.tga");
  terrainORM = TextureUtils::LoadTexture("assets/textures/terrain/ORM.tga");
  terrainNormal =
      TextureUtils::LoadTexture("assets/textures/terrain/Normal.tga");

  // --------------- TERRAIN GENERATION ------------------
  const uint16_t sz = TerrainSize;  // Terrain resolution
  std::vector<uint8_t> hdata(sz * sz);

  for (int y = 0; y < sz; ++y) {
    for (int x = 0; x < sz; ++x) {
      float nx = (x - sz * 0.5f) / (sz * 0.5f);  // [-1, 1]
      float ny = (y - sz * 0.5f) / (sz * 0.5f);  // [-1, 1]

      float dx = fabsf(nx);

      // Central erosion-shaped canyon (wider falloff)
      float canyonDepth = smoothStep(0.1f, 0.5f, dx);
      canyonDepth = 1.0f - canyonDepth;

      // Erosion-based slope smoothing using exponential decay
      float canyon = bx::clamp(expf(-15.0f * dx * dx), 0.0f, 1.0f);

      // Combine: canyon has base flattening and sloped edges
      float height = -0.6f * canyonDepth + 0.2f * canyon;

      // Smooth undulation (no sharp sin/cos anymore)
      float broadWaves = sinf(nx * bx::kPi * 2.0f) * cosf(ny * bx::kPi * 2.0f);
      height += broadWaves * 0.05f;

      // Noise (small amplitude)
      float noise = ((rand() % 100) / 100.0f - 0.5f) * 0.02f;
      height += noise;

      height = bx::clamp(height, -1.0f, 1.0f);
      hdata[y * sz + x] = uint8_t(127 + 127 * height);
    }
  }

  // Send to GPU as heightmap texture
  _heightTexture =
      bgfx::createTexture2D(sz, sz, false, 1, bgfx::TextureFormat::R8, 0,
                            bgfx::copy(hdata.data(), hdata.size()));

  // Rebuild terrain mesh grid
  terrainVertices.clear();
  terrainIndices.clear();

  for (uint16_t y = 0; y < sz; ++y) {
    for (uint16_t x = 0; x < sz; ++x) {
      // Use height from the texture (heightmap) to set vertex elevation
      float height = 0.0f;
      // Normalize the height
      terrainVertices.push_back({float(x), height, float(y),
                                 float(x) / (sz - 1), float(y) / (sz - 1)});
    }
  }

  for (uint16_t y = 0; y < sz - 1; ++y) {
    for (uint16_t x = 0; x < sz - 1; ++x) {
      uint16_t i = y * sz + x;
      terrainIndices.push_back(i);
      terrainIndices.push_back(i + 1);
      terrainIndices.push_back(i + sz);
      terrainIndices.push_back(i + 1);
      terrainIndices.push_back(i + sz + 1);
      terrainIndices.push_back(i + sz);
    }
  }

  // recreate our buffers after pushing the vertices
  _terrainVbh = bgfx::createVertexBuffer(
      bgfx::copy(terrainVertices.data(),
                 terrainVertices.size() * sizeof(PosTexCoord0Vertex)),
      PosTexCoord0Vertex::layout);

  _terrainIbh = bgfx::createIndexBuffer(bgfx::copy(
      terrainIndices.data(), terrainIndices.size() * sizeof(uint16_t)));

  // Create shadow vertex buffer

  std::vector<ShadowVertex> shadowVertices;
  shadowVertices.reserve(terrainVertices.size());

  for (const auto& v : terrainVertices) {
    shadowVertices.push_back({v.x, v.y, v.z, v.u, v.v});
  }

  _shadowVbh = bgfx::createVertexBuffer(
      bgfx::copy(shadowVertices.data(),
                 shadowVertices.size() * sizeof(ShadowVertex)),
      ShadowVertex::layout);

  createSkyboxBuffers();

  // Shadow Map initialization

  // 2048x2048 shadow map
  shadowMapTexture = bgfx::createTexture2D(
      2048, 2048, false, 1, bgfx::TextureFormat::RGBA8,
      BGFX_TEXTURE_RT | BGFX_TEXTURE_BLIT_DST);  // no depth format

  shadowMapFB = bgfx::createFrameBuffer(1, &shadowMapTexture, true);

  Logger::getInstance().Log(
      LogLevel::Debug,
      std::string("[Shadow] shadowMapTexture valid=") +
          (bgfx::isValid(shadowMapTexture) ? "true" : "false") +
          ", handle=" + std::to_string(shadowMapTexture.idx));

  Logger::getInstance().Log(
      LogLevel::Debug, std::string("[Shadow] shadowMapFB valid=") +
                           (bgfx::isValid(shadowMapFB) ? "true" : "false") +
                           ", handle=" + std::to_string(shadowMapFB.idx));
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
  cameraSystem.UpdateFromKeyboard();

  /* -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - */
  if (!bgfx::isValid(_terrainProgramHeight))
    return;

  // animate height map & day/night timer
  _cycleTime += 0.0007f;
  const float TWO_PI = bx::kPi * 2.0f;
  _cycleTime = fmod(_cycleTime, TWO_PI);  // Keep cycle time between [0, 2π]

  const uint16_t sz = 128;
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
  // Ensure views are active
  bgfx::touch(ViewID::SHADOW_PASS);
  bgfx::touch(ViewID::SCENE);
  bgfx::touch(ViewID::SCENE_N(1));

  bx::mtxIdentity(world_matrix);  // Reset each frame
  bx::mtxScale(world_matrix, TERRAIN_GLOBAL_SCALE, TERRAIN_GLOBAL_SCALE,
               TERRAIN_GLOBAL_SCALE);

  // --- Camera view and projection ---
  float view[16], proj[16];
  cameraSystem.GetViewMatrix(view);
  float aspect = canvasViewportW > 0 && canvasViewportH > 0
                     ? float(canvasViewportW) / canvasViewportH
                     : 1.0f;
  cameraSystem.GetProjectionMatrix(proj, aspect, 60.0f);

  // render sky into SCENE (view 1) and terrain into SCENE_N(1) (view 2):
  constexpr uint8_t skyView = ViewID::SCENE;
  constexpr uint8_t terrainView = ViewID::SCENE_N(1);
  constexpr uint8_t shadowView = ViewID::SHADOW_PASS;  //  new shadow pass view

  // upload camera matrices into both views:
  bgfx::setViewTransform(skyView, view, proj);
  bgfx::setViewTransform(terrainView, view, proj);

  // Animate the sun in a vertical arc (semi-circle)
  const float phi = _cycleTime;

  // Create a full sun arc from horizon to horizon
  const float x = cosf(phi);
  const float y = sinf(phi);
  const float z = sinf(phi) * 0.0f;

  const bx::Vec3 sunDir = bx::normalize(bx::Vec3(x, y, z));

  float dir[4] = {sunDir.x, sunDir.y, sunDir.z, 0.0f};
  float time[4] = {_cycleTime, 0, 0, 0};

  // Clamp between 0 and 1 — 0 when sun is below, 1 when fully overhead
  float t = bx::clamp(sunDir.y * 0.5f + 0.5f, 0.0f, 1.0f);

  // Soften sun color range
  float sunEnergy = bx::clamp(sunDir.y * 2.0f, 0.0f, 2.0f);  // brighter at noon

  _sunColorArray[0] = bx::lerp(1.0f, 1.0f, t) * sunEnergy;  // R
  _sunColorArray[1] = bx::lerp(0.5f, 1.0f, t) * sunEnergy;  // G
  _sunColorArray[2] = bx::lerp(0.1f, 1.0f, t) * sunEnergy;  // B
  _sunColorArray[3] = 0.0f;

  _parametersArray[0] = 0.01f;       // Sun size
  _parametersArray[1] = 3.0f;        // Bloom factor
  _parametersArray[2] = 1.0f;        // Exposure
  _parametersArray[3] = _cycleTime;  // Time

  // Sky ambient light
  _skyAmbientArray[0] = bx::lerp(0.05f, 0.3f, t);  // R
  _skyAmbientArray[1] = bx::lerp(0.1f, 0.4f, t);   // G
  _skyAmbientArray[2] = bx::lerp(0.2f, 0.5f, t);   // B
  _skyAmbientArray[3] = 0.0f;

  const float ambientBoost = 2000.0f;  // we can try different values here
  float boostedAmbient[4] = {
      _skyAmbientArray[0] * ambientBoost, _skyAmbientArray[1] * ambientBoost,
      _skyAmbientArray[2] * ambientBoost, _skyAmbientArray[3]};

  bgfx::setUniform(_skyAmbientUniform, boostedAmbient);

  // --- SHADOW PASS: render depth from light’s point of view ---
  // World center of terrain
  bx::Vec3 lightTarget = bx::Vec3(TerrainExtent, 0.0f, TerrainExtent);

  // Position sun back far enough
  bx::Vec3 lightPos =
      bx::mad(sunDir, -300.0f, lightTarget);  // Pull it back more

  float lightView[16], lightProj[16];
  bx::mtxLookAt(lightView, lightPos, lightTarget);

  // Much wider orthographic box
  float orthoSize =
      TerrainExtent * 2.0f * TERRAIN_GLOBAL_SCALE;  // Cover the SCALED terrain
  bx::mtxOrtho(lightProj, -orthoSize, orthoSize, -orthoSize, orthoSize, -500.0f,
               500.0f, 0, bgfx::getCaps()->homogeneousDepth);

  // Multiply view and projection
  float lightVP[16];
  bx::mtxMul(lightVP, lightProj, lightView);

  // Apply model transform
  float lightWorldVP[16];
  bx::mtxMul(lightWorldVP, lightVP, world_matrix);

  // Bias matrix from NDC [-1,1] to texture space [0,1]
  const float bias[16] = {
      0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f,
  };

  // Final biased light projection matrix
  float biasedLightVP[16];
  bx::mtxMul(biasedLightVP, bias, lightWorldVP);

  // Upload to shader *before* terrain or skybox
  bgfx::setUniform(_lightMatrixUniform, biasedLightVP);

  // Set view
  bgfx::setViewRect(shadowView, 0, 0, 2048, 2048);
  bgfx::setViewFrameBuffer(shadowView, shadowMapFB);
  bgfx::setViewTransform(shadowView, lightView, lightProj);
  bgfx::setViewClear(
      shadowView,
      BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,  // Use BGFX_CLEAR_COLOR
      0xffffffff,  // Clear color to white (represents max depth if your packing
                   // works that way)
      1.0f,        // Depth clear value
      0);          // Stencil clear value

  bgfx::setTransform(world_matrix);  // sends model matrix

  // Submit terrain to shadow map
  if (bgfx::isValid(_shadowVbh) && bgfx::isValid(_terrainIbh) &&
      bgfx::isValid(_terrainShadowProgram)) {

    bgfx::setVertexBuffer(0, _shadowVbh);
    bgfx::setIndexBuffer(_terrainIbh);
    bgfx::setState(BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
    bgfx::setTexture(0, _heightUniform, _heightTexture);
    bgfx::submit(shadowView, _terrainShadowProgram);
  }

  // --- SKYBOX (procedural full-screen quad) ---
  if (bgfx::isValid(_skyProgram)) {
    float invView[16], invProj[16];
    bx::mtxInverse(invView, view);
    bx::mtxInverse(invProj, proj);

    // View & proj are identity for screen-space quad
    float identity[16];
    bx::mtxIdentity(identity);
    bgfx::setTransform(identity);

    // Submit fullscreen quad
    bgfx::setVertexBuffer(0, _skyVbh);
    bgfx::setIndexBuffer(_skyIbh);

    bgfx::setUniform(_viewInvUniform, invView);
    bgfx::setUniform(_projInvUniform, invProj);
    bgfx::setUniform(_timeUniform, time);
    bgfx::setUniform(_sunDirUniform, dir);
    bgfx::setUniform(_sunLumUniform, _sunColorArray);
    bgfx::setUniform(_paramsUniform, _parametersArray);

    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_DEPTH_TEST_ALWAYS);

    skyLighting.Update(sunDir, _cycleTime);  // from skybox sun animation
    skyLighting.ApplyUniforms();

    bgfx::submit(skyView, _skyProgram);
  }

  // --- TERRAIN ---

  bgfx::setTransform(world_matrix);
  bgfx::setVertexBuffer(0, _terrainVbh);
  bgfx::setIndexBuffer(_terrainIbh);

  // Textures
  bgfx::setTexture(0, _heightUniform, _heightTexture);
  bgfx::setTexture(1, _s_diffuseUniform, terrainDiffuse);
  bgfx::setTexture(2, _s_ormUniform, terrainORM);
  bgfx::setTexture(3, _s_normalUniform, terrainNormal);
  bgfx::setTexture(4, _shadowSamplerUniform,
                   shadowMapTexture);  // bind shadow map

  // Shared lighting
  bgfx::setUniform(_sunLumUniform, _sunColorArray);
  bgfx::setUniform(_lightMatrixUniform, biasedLightVP);

  float cameraPos[4] = {};
  camera.getPosition(cameraPos);
  cameraPos[3] = 0.0f;
  bgfx::setUniform(_cameraPosUniform, cameraPos);

  // --- Render state setup ---
  uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS |
                   BGFX_STATE_MSAA;

  bgfx::setState(state);

  bgfx::setTransform(world_matrix);  // sends model matrix

  // Submit terrain pass
  bgfx::submit(terrainView, _terrainProgramHeight);
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
  destroy(_terrainShadowProgram);
  destroy(_skyProgram);

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
  destroy(terrainDiffuse);
  destroy(terrainORM);
  destroy(terrainNormal);

  destroy(_s_diffuseUniform);
  destroy(_s_ormUniform);
  destroy(_s_normalUniform);
  destroy(_lightMatrixUniform);
  destroy(_skyAmbientUniform);
  destroy(_shadowSamplerUniform);
  destroy(_cameraPosUniform);
  destroy(shadowMapTexture);
  destroy(shadowMapFB);
  destroy(_shadowVbh);

  destroy(_skyVbh);
  destroy(_skyIbh);
}
