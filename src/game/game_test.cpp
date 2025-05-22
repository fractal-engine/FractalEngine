#include "game/game_test.h"  // Make sure this includes the PosTexCoord0Vertex definition

#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <vector>

#include "core/logger.h"
#include "core/view_ids.h"
#include "renderer/shaders/shader_utils.h"
#include "subsystem/subsystem_manager.h"
#include "tools/texture_utils.h"

// --- Local Math Workarounds ---
inline float local_min(float _a, float _b) {
  return _a < _b ? _a : _b;
}

inline float local_max(float _a, float _b) {
  return _a > _b ? _a : _b;
}

inline float local_clamp(float _val, float _min_val, float _max_val) {
  return local_max(_min_val, local_min(_val, _max_val));
}

inline float local_smoothStep(float edge0, float edge1, float x) {
  float t = local_clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}
// --- End Local Math Workarounds ---

// Definition of the static member variable for PosTexCoord0Vertex
bgfx::VertexLayout PosTexCoord0Vertex::layout;

constexpr float TERRAIN_MAX_ACTUAL_HEIGHT = 150.0f;
constexpr uint8_t SHADOW_MAP_VIEW_ID = ViewID::SHADOW_PASS;
constexpr uint16_t KNOWN_SHADOW_MAP_SIZE = 2048;

struct ScreenPosVertex {
  float x, y;
  static bgfx::VertexLayout layout;
};
bgfx::VertexLayout ScreenPosVertex::layout;  // Also define for ScreenPosVertex
                                             // if not done elsewhere

void PosTexCoord0Vertex::init() {
  // The definition 'PosTexCoord0Vertex::layout;' ensures memory is allocated.
  // Now we initialize its content.
  PosTexCoord0Vertex::layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .end();
}

GameTest::GameTest()
    : camera(),
      cameraSystem(&camera),
      _terrainProgramHeight(BGFX_INVALID_HANDLE),
      _heightUniform(BGFX_INVALID_HANDLE),
      _heightTexture(BGFX_INVALID_HANDLE),
      terrainDiffuse(BGFX_INVALID_HANDLE),
      terrainORM(BGFX_INVALID_HANDLE),
      terrainNormal(BGFX_INVALID_HANDLE),
      _cameraPosUniform(BGFX_INVALID_HANDLE),
      _terrainVbh(BGFX_INVALID_HANDLE),
      _terrainIbh(BGFX_INVALID_HANDLE),
      _terrainParamsUniform(BGFX_INVALID_HANDLE),
      _heightmapTexelSizeUniform(BGFX_INVALID_HANDLE),
      _skyProgram(BGFX_INVALID_HANDLE),
      _skyVbh(BGFX_INVALID_HANDLE),
      _skyIbh(BGFX_INVALID_HANDLE),
      _timeUniform(BGFX_INVALID_HANDLE),
      _sunDirUniform(BGFX_INVALID_HANDLE),
      _sunLumUniform(BGFX_INVALID_HANDLE),
      _paramsUniform(BGFX_INVALID_HANDLE),
      _viewInvUniform(BGFX_INVALID_HANDLE),
      _projInvUniform(BGFX_INVALID_HANDLE),
      _s_diffuseUniform(BGFX_INVALID_HANDLE),
      _s_ormUniform(BGFX_INVALID_HANDLE),
      _s_normalUniform(BGFX_INVALID_HANDLE),
      _skyAmbientUniform(BGFX_INVALID_HANDLE),
      _lightMatrixUniform(BGFX_INVALID_HANDLE),
      _shadowSamplerUniform(BGFX_INVALID_HANDLE),
      shadowMapTexture(BGFX_INVALID_HANDLE),
      shadowMapFB(BGFX_INVALID_HANDLE),
      _terrainShadowProgram(BGFX_INVALID_HANDLE),
      _cycleTime(0.0f) {
  bx::mtxIdentity(world_matrix);
  _skyAmbientArray[0] = 0.1f;
  _skyAmbientArray[1] = 0.1f;
  _skyAmbientArray[2] = 0.1f;
  _skyAmbientArray[3] = 0.0f;
}

GameTest::~GameTest() {
  // Shutdown handles cleanup
}

void GameTest::Init() {
  Logger::getInstance().Log(LogLevel::Debug, "[GameTest] Init() called.");

  PosTexCoord0Vertex::init();  // Call the static init method to set up the
                               // layout's attributes

  // Ensure ScreenPosVertex layout is also initialized if you use it
  // ScreenPosVertex::layout might be defined and initialized elsewhere if it's
  // a more common struct If ScreenPosVertex::layout is specific to GameTest,
  // its definition and init should be here too.
  if (!ScreenPosVertex::layout.m_stride) {  // Check if not already initialized
    ScreenPosVertex::layout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .end();
  }

  float terrainGridSize = (TerrainSize - 1) * TerrainScale;
  float terrainCenterPos[3] = {terrainGridSize * 0.5f, 0.0f,
                               terrainGridSize * 0.5f};
  camera.setTarget(terrainCenterPos);
  camera.setDistance(TerrainScale * 20.0f);
  camera.setPitch(bx::toRad(30.0f));
  camera.setYaw(bx::toRad(45.0f));

  auto& shaderMgr = *SubsystemManager::GetShaderManager();
  _terrainProgramHeight =
      shaderMgr.LoadProgram("terrain_pbr", "vs_terrain.bin", "fs_terrain.bin");
  _skyProgram =
      shaderMgr.LoadProgram("skybox_proc", "vs_skybox.bin", "fs_skybox.bin");
  _terrainShadowProgram = shaderMgr.LoadProgram(
      "terrain_shadow_depth", "vs_shadow.bin", "fs_shadow.bin");

  _heightUniform =
      bgfx::createUniform("s_heightTexture", bgfx::UniformType::Sampler);
  _s_diffuseUniform =
      bgfx::createUniform("s_diffuse", bgfx::UniformType::Sampler);
  _s_ormUniform = bgfx::createUniform("s_orm", bgfx::UniformType::Sampler);
  _s_normalUniform =
      bgfx::createUniform("s_normal", bgfx::UniformType::Sampler);
  _shadowSamplerUniform =
      bgfx::createUniform("s_shadowMap", bgfx::UniformType::Sampler);

  _terrainParamsUniform =
      bgfx::createUniform("u_terrainParams", bgfx::UniformType::Vec4);
  _heightmapTexelSizeUniform =
      bgfx::createUniform("u_heightmapTexelSize", bgfx::UniformType::Vec4);
  _cameraPosUniform =
      bgfx::createUniform("u_cameraPos", bgfx::UniformType::Vec4);
  _lightMatrixUniform =
      bgfx::createUniform("u_lightMatrix", bgfx::UniformType::Mat4, 1);

  _sunDirUniform =
      bgfx::createUniform("u_sunDirection", bgfx::UniformType::Vec4);
  _sunLumUniform =
      bgfx::createUniform("u_sunLuminance", bgfx::UniformType::Vec4);
  _skyAmbientUniform =
      bgfx::createUniform("u_skyAmbient", bgfx::UniformType::Vec4);

  _paramsUniform = bgfx::createUniform("u_parameters", bgfx::UniformType::Vec4);
  _viewInvUniform = bgfx::createUniform("u_viewInv", bgfx::UniformType::Mat4);
  _projInvUniform = bgfx::createUniform("u_projInv", bgfx::UniformType::Mat4);

  terrainDiffuse =
      TextureUtils::LoadTexture("assets/textures/terrain/basecolor.tga");
  terrainORM = TextureUtils::LoadTexture("assets/textures/terrain/ORM.tga");
  terrainNormal =
      TextureUtils::LoadTexture("assets/textures/terrain/Normal.tga");

  const uint16_t hm_sz = TerrainSize;
  std::vector<uint8_t> heightmapData(hm_sz * hm_sz);
  for (uint16_t y = 0; y < hm_sz; ++y) {
    for (uint16_t x = 0; x < hm_sz; ++x) {
      float nx = (float(x) / float(hm_sz - 1) - 0.5f) * 2.0f;
      float ny = (float(y) / float(hm_sz - 1) - 0.5f) * 2.0f;
      float dist = bx::sqrt(nx * nx + ny * ny);
      float h_base = bx::cos(dist * 10.0f) * local_smoothStep(1.0f, 0.5f, dist);
      float h_detail = (bx::sin(nx * 5.0f + _cycleTime * 0.1f) * 0.5f +
                        bx::cos(ny * 5.0f + _cycleTime * 0.1f) * 0.5f);
      float h = (h_base + h_detail) / 2.0f;
      heightmapData[y * hm_sz + x] =
          uint8_t(local_clamp(h * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f);
    }
  }
  _heightTexture = bgfx::createTexture2D(
      hm_sz, hm_sz, false, 1, bgfx::TextureFormat::R8,
      BGFX_TEXTURE_NONE | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
      bgfx::copy(heightmapData.data(), heightmapData.size()));

  terrainVertices.clear();
  terrainIndices.clear();

  for (uint16_t y = 0; y < TerrainSize; ++y) {
    for (uint16_t x = 0; x < TerrainSize; ++x) {
      terrainVertices.push_back({(float)x, 0.0f, (float)y,
                                 (float)x / (TerrainSize - 1),
                                 (float)y / (TerrainSize - 1)});
    }
  }

  for (uint16_t y = 0; y < TerrainSize - 1; ++y) {
    for (uint16_t x = 0; x < TerrainSize - 1; ++x) {
      uint16_t i = y * TerrainSize + x;
      terrainIndices.push_back(i);
      terrainIndices.push_back(i + TerrainSize);
      terrainIndices.push_back(i + 1);

      terrainIndices.push_back(i + 1);
      terrainIndices.push_back(i + TerrainSize);
      terrainIndices.push_back(i + TerrainSize + 1);
    }
  }

  _terrainVbh = bgfx::createVertexBuffer(
      bgfx::copy(terrainVertices.data(),
                 terrainVertices.size() * sizeof(PosTexCoord0Vertex)),
      PosTexCoord0Vertex::layout  // Use the now defined and initialized layout
  );
  _terrainIbh = bgfx::createIndexBuffer(bgfx::copy(
      terrainIndices.data(), terrainIndices.size() * sizeof(uint16_t)));

  createSkyboxBuffers();

  shadowMapTexture = bgfx::createTexture2D(
      KNOWN_SHADOW_MAP_SIZE, KNOWN_SHADOW_MAP_SIZE, false, 1,
      bgfx::TextureFormat::D16,
      BGFX_TEXTURE_RT_WRITE_ONLY | BGFX_SAMPLER_COMPARE_LESS);
  shadowMapFB = bgfx::createFrameBuffer(1, &shadowMapTexture, true);

  bx::mtxScale(this->world_matrix, TerrainScale, 1.0f, TerrainScale);
}

void GameTest::createSkyboxBuffers() {
  static const ScreenPosVertex quadVertices[] = {
      {
          -1.0f,
          1.0f,
      },
      {
          1.0f,
          1.0f,
      },
      {
          -1.0f,
          -1.0f,
      },
      {
          1.0f,
          -1.0f,
      },
  };
  static const uint16_t quadIndices[] = {0, 2, 1, 1, 2, 3};

  _skyVbh = bgfx::createVertexBuffer(
      bgfx::makeRef(quadVertices, sizeof(quadVertices)),
      ScreenPosVertex::layout  // Use the ScreenPosVertex layout
  );
  _skyIbh =
      bgfx::createIndexBuffer(bgfx::makeRef(quadIndices, sizeof(quadIndices)));
}

void GameTest::Update() {
  cameraSystem.UpdateFromKeyboard();

  if (!bgfx::isValid(_terrainProgramHeight)) {
    return;
  }

  _cycleTime += 0.0007f;
  if (_cycleTime > bx::kPi * 2.0f) {
    _cycleTime -= bx::kPi * 2.0f;
  }

  const uint16_t hm_sz = TerrainSize;
  std::vector<uint8_t> heightmap_dynamic_data(hm_sz * hm_sz);
  for (uint16_t y = 0; y < hm_sz; ++y) {
    for (uint16_t x = 0; x < hm_sz; ++x) {
      float nx = (float(x) / float(hm_sz - 1) - 0.5f) * 2.0f;
      float ny = (float(y) / float(hm_sz - 1) - 0.5f) * 2.0f;
      float base_h = bx::sin(nx * 3.0f + _cycleTime * 0.5f) *
                     bx::cos(ny * 3.0f - _cycleTime * 0.3f);
      float detail_h = bx::sin((nx + _cycleTime * 0.01f) * 10.f) *
                       bx::cos((ny - _cycleTime * 0.01f) * 10.f) * 0.3f;
      float final_h = (base_h + detail_h) * 0.5f + 0.5f;
      heightmap_dynamic_data[y * hm_sz + x] =
          uint8_t(local_clamp(final_h, 0.0f, 1.0f) * 255.0f);
    }
  }
  if (bgfx::isValid(_heightTexture)) {
    bgfx::updateTexture2D(_heightTexture, 0, 0, 0, 0, hm_sz, hm_sz,
                          bgfx::copy(heightmap_dynamic_data.data(),
                                     heightmap_dynamic_data.size()));
  }
}

void GameTest::Render() {
  if (!bgfx::isValid(_terrainProgramHeight) || !bgfx::isValid(_skyProgram) ||
      !bgfx::isValid(_terrainShadowProgram)) {
    return;
  }

  float viewMatrix[16], projMatrix[16];
  camera.getViewMatrix(viewMatrix);
  float aspectRatio = (canvasViewportW > 0 && canvasViewportH > 0)
                          ? float(canvasViewportW) / float(canvasViewportH)
                          : 1.0f;
  camera.getProjectionMatrix(projMatrix, aspectRatio);

  float sunAngle = _cycleTime;
  bx::Vec3 sunDirectionVec =
      bx::normalize(bx::Vec3(bx::cos(sunAngle), bx::sin(sunAngle), 0.1f));
  if (sunDirectionVec.y < -0.2f)
    sunDirectionVec.y = -0.2f;

  float sunDirShader[4] = {sunDirectionVec.x, sunDirectionVec.y,
                           sunDirectionVec.z, 0.0f};

  float sunElevationFactor = local_smoothStep(-0.15f, 0.2f, sunDirectionVec.y);
  float sunIntensity = bx::lerp(0.3f, 3.0f, sunElevationFactor);

  _sunColorArray[0] = bx::lerp(0.8f, 1.0f, sunElevationFactor) * sunIntensity;
  _sunColorArray[1] = bx::lerp(0.6f, 1.0f, sunElevationFactor) * sunIntensity;
  _sunColorArray[2] = bx::lerp(0.4f, 1.0f, sunElevationFactor) * sunIntensity;
  _sunColorArray[3] = 0.0f;

  _skyAmbientArray[0] = bx::lerp(0.02f, 0.3f, sunElevationFactor);
  _skyAmbientArray[1] = bx::lerp(0.03f, 0.4f, sunElevationFactor);
  _skyAmbientArray[2] = bx::lerp(0.05f, 0.55f, sunElevationFactor);
  _skyAmbientArray[3] = 0.0f;

  _parametersArray[0] = 0.005f;
  _parametersArray[1] = 0.1f;
  _parametersArray[2] = 1.0f;
  _parametersArray[3] = _cycleTime;

  bgfx::setViewFrameBuffer(SHADOW_MAP_VIEW_ID, shadowMapFB);
  bgfx::setViewRect(SHADOW_MAP_VIEW_ID, 0, 0, KNOWN_SHADOW_MAP_SIZE,
                    KNOWN_SHADOW_MAP_SIZE);
  bgfx::setViewClear(SHADOW_MAP_VIEW_ID, BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);

  float lightViewMatrix[16], lightProjMatrix[16];
  float terrainWorldSize = (TerrainSize - 1) * TerrainScale;
  bx::Vec3 actualLightTarget = {terrainWorldSize * 0.5f,
                                TERRAIN_MAX_ACTUAL_HEIGHT * 0.25f,
                                terrainWorldSize * 0.5f};
  bx::Vec3 lightPos =
      bx::mad(sunDirectionVec, -terrainWorldSize * 1.5f, actualLightTarget);
  bx::mtxLookAt(lightViewMatrix, lightPos, actualLightTarget,
                bx::Vec3(0.0f, 1.0f, 0.0f));

  float orthoHalfSize = terrainWorldSize * 0.6f;
  bx::mtxOrtho(lightProjMatrix, -orthoHalfSize, orthoHalfSize, -orthoHalfSize,
               orthoHalfSize, 0.1f, terrainWorldSize * 3.0f, 0.0f,
               bgfx::getCaps()->homogeneousDepth);

  bgfx::setViewTransform(SHADOW_MAP_VIEW_ID, lightViewMatrix, lightProjMatrix);

  float terrainParamsArr[4] = {TERRAIN_MAX_ACTUAL_HEIGHT, 0.0f, 1.0f, 1.0f};
  bgfx::setUniform(_terrainParamsUniform, terrainParamsArr);
  bgfx::setTexture(0, _heightUniform, _heightTexture);

  bgfx::setTransform(this->world_matrix);
  bgfx::setVertexBuffer(0, _terrainVbh);
  bgfx::setIndexBuffer(_terrainIbh);
  bgfx::setState(BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS |
                 BGFX_STATE_CULL_CW | BGFX_STATE_MSAA);
  bgfx::submit(SHADOW_MAP_VIEW_ID, _terrainShadowProgram);

  bgfx::setViewRect(ViewID::SCENE, 0, 0, canvasViewportW, canvasViewportH);
  bgfx::setViewClear(ViewID::SCENE, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     0x303030ff, 1.0f, 0);

  float invViewMatrix[16], invProjMatrix[16];
  bx::mtxInverse(invViewMatrix, viewMatrix);
  bx::mtxInverse(invProjMatrix, projMatrix);

  bgfx::setViewTransform(ViewID::SCENE, viewMatrix, projMatrix);

  float skyboxModelMatrix[16];
  bx::mtxIdentity(skyboxModelMatrix);
  bgfx::setTransform(skyboxModelMatrix);

  bgfx::setVertexBuffer(0, _skyVbh);
  bgfx::setIndexBuffer(_skyIbh);

  bgfx::setUniform(_viewInvUniform, invViewMatrix);
  bgfx::setUniform(_projInvUniform, invProjMatrix);
  bgfx::setUniform(_sunDirUniform, sunDirShader);
  bgfx::setUniform(_sunLumUniform, _sunColorArray);
  bgfx::setUniform(_paramsUniform, _parametersArray);

  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                 BGFX_STATE_DEPTH_TEST_LEQUAL | BGFX_STATE_CULL_CW);
  bgfx::submit(ViewID::SCENE, _skyProgram);

  uint8_t terrainViewID = ViewID::SCENE_N(1);
  bgfx::setViewRect(terrainViewID, 0, 0, canvasViewportW, canvasViewportH);
  bgfx::setViewTransform(terrainViewID, viewMatrix, projMatrix);

  bgfx::setTransform(this->world_matrix);

  float camPos[4];
  camera.getPosition(camPos);
  camPos[3] = 1.0f;
  bgfx::setUniform(_cameraPosUniform, camPos);

  bgfx::setUniform(_sunDirUniform, sunDirShader);
  bgfx::setUniform(_sunLumUniform, _sunColorArray);
  bgfx::setUniform(_skyAmbientUniform, _skyAmbientArray);

  float biasMatrix[16] = {
      0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f,
  };
  float lightVPMatrix[16];
  bx::mtxMul(lightVPMatrix, lightProjMatrix, lightViewMatrix);
  float finalLightMatrixForShader[16];
  bx::mtxMul(finalLightMatrixForShader, biasMatrix, lightVPMatrix);
  bgfx::setUniform(_lightMatrixUniform, finalLightMatrixForShader);

  bgfx::setUniform(_terrainParamsUniform, terrainParamsArr);

  float hmTexelSize[4] = {1.0f / TerrainSize, 1.0f / TerrainSize, 0.0f, 0.0f};
  bgfx::setUniform(_heightmapTexelSizeUniform, hmTexelSize);

  bgfx::setTexture(0, _s_diffuseUniform, terrainDiffuse);
  bgfx::setTexture(1, _s_ormUniform, terrainORM);
  bgfx::setTexture(2, _s_normalUniform, terrainNormal);
  bgfx::setTexture(3, _heightUniform, _heightTexture);
  bgfx::setTexture(4, _shadowSamplerUniform, shadowMapTexture);

  bgfx::setVertexBuffer(0, _terrainVbh);
  bgfx::setIndexBuffer(_terrainIbh);
  bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_CULL_CW | BGFX_STATE_MSAA);
  bgfx::submit(terrainViewID, _terrainProgramHeight);
}

void GameTest::Shutdown() {
  Logger::getInstance().Log(LogLevel::Debug, "[GameTest] Shutdown() called.");

  auto destroyHandle = [](auto& handle) {
    if (bgfx::isValid(handle)) {
      bgfx::destroy(handle);
      handle = BGFX_INVALID_HANDLE;
    }
  };

  destroyHandle(_terrainProgramHeight);
  destroyHandle(_skyProgram);
  destroyHandle(_terrainShadowProgram);
  destroyHandle(_heightTexture);
  destroyHandle(terrainDiffuse);
  destroyHandle(terrainORM);
  destroyHandle(terrainNormal);
  destroyHandle(shadowMapFB);
  destroyHandle(_terrainVbh);
  destroyHandle(_terrainIbh);
  destroyHandle(_skyVbh);
  destroyHandle(_skyIbh);
  destroyHandle(_heightUniform);
  destroyHandle(_s_diffuseUniform);
  destroyHandle(_s_ormUniform);
  destroyHandle(_s_normalUniform);
  destroyHandle(_shadowSamplerUniform);
  destroyHandle(_terrainParamsUniform);
  destroyHandle(_heightmapTexelSizeUniform);
  destroyHandle(_cameraPosUniform);
  destroyHandle(_lightMatrixUniform);
  destroyHandle(_sunDirUniform);
  destroyHandle(_sunLumUniform);
  destroyHandle(_skyAmbientUniform);
  destroyHandle(_paramsUniform);
  destroyHandle(_viewInvUniform);
  destroyHandle(_projInvUniform);
}