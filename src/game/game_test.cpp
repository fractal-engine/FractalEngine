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

  _uModelUniform = bgfx::createUniform("u_model", bgfx::UniformType::Mat4, 1);

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
      // Normalize the coordinates to [-1, 1] for terrain manipulation
      float nx = (x - sz / 2.0f) / (sz / 2.0f);
      float ny = (y - sz / 2.0f) / (sz / 2.0f);

      // Canyon-like depth carved along X (stronger canyon in the center)
      // The canyon is deeper and steeper around the center, simulating the
      // Grand Canyon
      float canyon =
          expf(-10.0f * nx * nx);  // Steep center, shallows out at the edges
      canyon =
          std::max(canyon, 0.2f);  // Ensure a minimum depth for visual realism

      // Combine with sine and cosine functions for undulating terrain (canyon
      // ridges, mesas)
      float height = (sinf(x * 0.1f) + cosf(y * 0.1f)) * 0.5f;

      // Simulate more varied terrain with the canyon effect
      height *= canyon;

      // Add some noise or randomness for variation (representing erosion,
      // randomness)
      float noise =
          (rand() % 100) / 100.0f;  // Random variation between 0.0 and 1.0
      height +=
          noise *
          0.1f;  // Slight variation in height to make terrain less uniform

      // Store the final height value in the heightmap (clamped to range)
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
    shadowVertices.push_back({v.x, v.y, v.z});
  }

  _shadowVbh = bgfx::createVertexBuffer(
      bgfx::copy(shadowVertices.data(),
                 shadowVertices.size() * sizeof(ShadowVertex)),
      ShadowVertex::layout);

  createSkyboxBuffers();

  // Shadow Map initialization

  // 2048x2048 shadow map, depth-only
  shadowMapTexture = bgfx::createTexture2D(
      2048, 2048, false, 1, bgfx::TextureFormat::D16,
      BGFX_TEXTURE_RT_WRITE_ONLY | BGFX_SAMPLER_COMPARE_LESS);

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
  _sunColorArray[0] = bx::lerp(1.0f, 1.0f, t);  // R stays 1.0
  _sunColorArray[1] = bx::lerp(0.5f, 1.0f, t);  // G warms up
  _sunColorArray[2] = bx::lerp(0.1f, 1.0f, t);  // B goes from warm to cool
  _sunColorArray[3] = 0.0f;

  _parametersArray[0] = 0.01f;       // Sun size
  _parametersArray[1] = 3.0f;        // Bloom factor
  _parametersArray[2] = 1.0f;        // Exposure
  _parametersArray[3] = _cycleTime;  // Time

  // --- SHADOW PASS: render depth from light’s point of view ---
  bx::Vec3 lightPos = bx::mul(sunDir, -100.0f);  // pull back from origin

  // Update light target to center of terrain using TerrainExtent
  bx::Vec3 lightTarget = {TerrainExtent, 0.0f, TerrainExtent};

  float lightView[16], lightProj[16], lightVP[16];
  bx::mtxLookAt(lightView, lightPos, lightTarget);

  bx::mtxOrtho(lightProj, -TerrainExtent, TerrainExtent, -TerrainExtent,
               TerrainExtent, -TerrainExtent, TerrainExtent, 0, false);

  bx::mtxMul(lightVP, lightProj, lightView);

  if (!bgfx::isValid(shadowMapFB)) {
    Logger::getInstance().Log(LogLevel::Error, "ShadowMapFB is invalid!");
    return;  // Skip render to avoid crash
  }

  bgfx::setViewRect(shadowView, 0, 0, 2048, 2048);
  bgfx::setViewFrameBuffer(shadowView, shadowMapFB);
  bgfx::setViewTransform(shadowView, lightView, lightProj);
  bgfx::setViewClear(shadowView, BGFX_CLEAR_DEPTH, 0, 1.0f, 0);

  // Scale the terrain using class-defined TerrainScale
  bx::mtxScale(world_matrix, TerrainScale, TerrainScale, TerrainScale);

  bgfx::setTransform(world_matrix);

  // Safely submit if all resources are valid
  if (bgfx::isValid(_shadowVbh) && bgfx::isValid(_terrainIbh) &&
      bgfx::isValid(_terrainShadowProgram)) {
    bgfx::setVertexBuffer(0, _shadowVbh);
    bgfx::setIndexBuffer(_terrainIbh);
    bgfx::setState(BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
    bgfx::submit(shadowView, _terrainShadowProgram);
  } else {
    Logger::getInstance().Log(LogLevel::Error,
                              "Shadow pass skipped — invalid resources.");
  }

  // Upload light matrix for main terrain pass
  bgfx::setUniform(_lightMatrixUniform, lightVP);

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
  bx::mtxScale(world_matrix, 5.f, 5.f, 5.f);
  bgfx::setTransform(world_matrix);
  bgfx::setVertexBuffer(0, _terrainVbh);
  bgfx::setIndexBuffer(_terrainIbh);

  // Textures
  bgfx::setTexture(0, _heightUniform, _heightTexture);
  bgfx::setTexture(1, _s_diffuseUniform, terrainDiffuse);
  bgfx::setTexture(2, _s_ormUniform, terrainORM);
  bgfx::setTexture(3, _s_normalUniform, terrainNormal);
  bgfx::setTexture(4, _shadowSamplerUniform, shadowMapTexture,
                   BGFX_SAMPLER_COMPARE_LESS);  //  bind shadow map

  // Shared lighting
  bgfx::setUniform(_sunLumUniform, _sunColorArray);
  bgfx::setUniform(_lightMatrixUniform, lightVP);

  float cameraPos[4] = {};
  camera.getPosition(cameraPos);
  cameraPos[3] = 0.0f;
  bgfx::setUniform(_cameraPosUniform, cameraPos);

  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                 BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS |
                 BGFX_STATE_MSAA);

  float modelArray[16 * 1] = {};
  memcpy(modelArray, world_matrix, sizeof(world_matrix));
  // bgfx::setUniform(_uModelUniform, modelArray, 1);  // count = 1 but
  // uncommenting causes a memory access error

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
  destroy(_uModelUniform);

  destroy(_skyVbh);
  destroy(_skyIbh);
}
