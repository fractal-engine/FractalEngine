#include "game/game_test.h"

#include <SDL.h>
#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <string.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>  // TODO: remove this?
#include <vector>

#include "editor/runtime/runtime.h"
#include "engine/core/logger.h"
#include "engine/core/view_ids.h"
#include "engine/ecs/ecs_collection.h"
#include "engine/pcg/constraints/biome_presets.h"
#include "engine/pcg/core/feature_descriptors.h"
#include "engine/pcg/terrain/terrain_generator.h"
#include "engine/renderer/graphics_renderer.h"
#include "engine/renderer/lighting/light.h"
#include "engine/resources/shader_utils.h"
#include "engine/resources/textures/texture_utils.h"
#include "engine/transform/transform.h"

/*******************************************************************************
 * TODO:
 * - Move bgfx uniforms to graphics renderer?
 * - Move load terrain textures to runtime.cpp (?)
 ******************************************************************************/

// Helper to make logging BGFX handles easier
/* std::string handle_to_string(bgfx::ProgramHandle h) {
  return h.idx == bgfx::kInvalidHandle ? "INVALID" : std::to_string(h.idx);
}
std::string handle_to_string(bgfx::VertexBufferHandle h) {
  return h.idx == bgfx::kInvalidHandle ? "INVALID" : std::to_string(h.idx);
}
std::string handle_to_string(bgfx::IndexBufferHandle h) {
  return h.idx == bgfx::kInvalidHandle ? "INVALID" : std::to_string(h.idx);
}
std::string handle_to_string(bgfx::TextureHandle h) {
  return h.idx == bgfx::kInvalidHandle ? "INVALID" : std::to_string(h.idx);
}
std::string handle_to_string(bgfx::UniformHandle h) {
  return h.idx == bgfx::kInvalidHandle ? "INVALID" : std::to_string(h.idx);
}
// --- Local Math Workarounds  ---
inline float local_min(float _a, float _b) {
  return _a < _b ? _a : _b;
}
inline float local_max(float a, float b) {
  return a > b ? a : b;
}
inline float local_clamp(float v, float m, float M) {
  return local_max(m, local_min(v, M));
}
inline float local_smoothStep(float e0, float e1, float x) {
  float t = local_clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
} */

// ──────────────────────────────────────────────────────
//  Vertex layouts
// ──────────────────────────────────────────────────────
bgfx::VertexLayout PosTexCoord0Vertex::layout;

// Maximum height of the terrain in world units
constexpr float TERRAIN_MAX_ACTUAL_HEIGHT = 150.0f;
// View ID for the shadow map rendering pass
// constexpr uint8_t SHADOW_MAP_VIEW_ID = ViewID::Shadow(0);
// Fixed size for the shadow map texture
constexpr uint16_t KNOWN_SHADOW_MAP_SIZE = 2048;

// Vertex structure for screen-space positions (e.g., full-screen quad)
struct ScreenPosVertex {
  float x, y;
  static bgfx::VertexLayout layout;
};

bgfx::VertexLayout
    ScreenPosVertex::layout;  // Definition for ScreenPosVertex layout

// Vertex structure for shadow map rendering (position and UVs for heightmap
// lookup)
struct ShadowVertex {
  float x, y, z;
  float u, v;
  static bgfx::VertexLayout layout;
};
bgfx::VertexLayout ShadowVertex::layout;  // Definition for ShadowVertex layout

// Initializes static vertex layouts
void PosTexCoord0Vertex::init() {
  // Initialize layout for PosTexCoord0Vertex (position and texture coordinates)
  PosTexCoord0Vertex::layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .end();

  // Initialize layout for ShadowVertex (position and texture coordinates for
  // shadow map)
  ShadowVertex::layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .end();
}

// --- GameTest Class Implementation ---

// Constructor: Initializes BGFX handles to invalid and sets default values
// TODO: comment out once all passes have been moved!
GameTest::GameTest()
    : _terrainProgramHeight(BGFX_INVALID_HANDLE),
      _heightUniform(BGFX_INVALID_HANDLE),
      _heightTexture(BGFX_INVALID_HANDLE),
      terrainORM(BGFX_INVALID_HANDLE),
      terrainNormal(BGFX_INVALID_HANDLE),
      _cameraPosUniform(BGFX_INVALID_HANDLE),
      terrainDiffuse(BGFX_INVALID_HANDLE),
      _terrainVbh(BGFX_INVALID_HANDLE),
      _terrainIbh(BGFX_INVALID_HANDLE),
      _terrainParamsUniform(BGFX_INVALID_HANDLE),
      _heightmapTexelSizeUniform(BGFX_INVALID_HANDLE),
      _shadowVbh(BGFX_INVALID_HANDLE),
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
      _scatterParamsUniform(BGFX_INVALID_HANDLE),
      _betaRUniform(BGFX_INVALID_HANDLE),
      _betaMUniform(BGFX_INVALID_HANDLE),
      _u_waterColor(BGFX_INVALID_HANDLE),
      _s_waterTexUniform(BGFX_INVALID_HANDLE),
      _s_waterNormUniform(BGFX_INVALID_HANDLE),
      _waterTex(BGFX_INVALID_HANDLE),
      _waterNormalTex(BGFX_INVALID_HANDLE),
      _terrainShadowProgram(BGFX_INVALID_HANDLE),
      grassDiffuse(BGFX_INVALID_HANDLE),
      grassORM(BGFX_INVALID_HANDLE),
      grassNormal(BGFX_INVALID_HANDLE),
      _s_grassDiffuseUniform(BGFX_INVALID_HANDLE),
      _s_grassORMUniform(BGFX_INVALID_HANDLE),
      _s_grassNormalUniform(BGFX_INVALID_HANDLE),
      _u_slopeBlendParamsUniform(BGFX_INVALID_HANDLE),
      generator_(PCG::Config{}) {
  bx::mtxIdentity(world_matrix);  // Initialize world matrix to identity
}

// Destructor: Cleanup is handled in Shutdown()
GameTest::~GameTest() {}
// ──────────────────────────────────────────────────────
//  Init()
// ──────────────────────────────────────────────────────
void GameTest::Init() {
  Logger::getInstance().Log(LogLevel::Debug, "GameTest: Init() called.");

  // Initialize vertex layouts
  PosTexCoord0Vertex::init();
  if (!ScreenPosVertex::layout.m_stride) {  // Initialize ScreenPosVertex layout
                                            // if not already done
    ScreenPosVertex::layout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .end();
  }

  // Load terrain textures
  terrainDiffuse =
      TextureUtils::LoadTexture("assets/textures/terrain/basecolor.tga");
  terrainORM = TextureUtils::LoadTexture("assets/textures/terrain/ORM.tga");
  terrainNormal =
      TextureUtils::LoadTexture("assets/textures/terrain/Normal.tga");
  // Load water textures
  _waterTex =
      TextureUtils::LoadTexture("assets/textures/water/water_diffuse.tga");
  _waterNormalTex =
      TextureUtils::LoadTexture("assets/textures/water/water_normal.tga");
  // Load grass textures
  grassDiffuse =
      TextureUtils::LoadTexture("assets/textures/grass/grass_diffuse.tga");
  grassORM = TextureUtils::LoadTexture("assets/textures/grass/grass_ORM.tga");
  grassNormal =
      TextureUtils::LoadTexture("assets/textures/grass/grass_normal.tga");

  if (!bgfx::isValid(grassDiffuse) || !bgfx::isValid(grassORM) ||
      !bgfx::isValid(grassNormal)) {
    Logger::getInstance().Log(
        LogLevel::Warning,
        "GameTest::Init: One or more grass textures failed to load.");
  }

  // Load shader programs
  auto& ShaderManager = *Runtime::Shader();

  /*  _terrainProgramHeight = ShaderManager.LoadProgram(
      "terrain", "vs_vertex_color.bin", "fs_vertex_color.bin");
  Logger::getInstance().Log(
      LogLevel::Debug,
      std::string("_terrainPBR valid = ") +
          (bgfx::isValid(_terrainProgramHeight) ? "true" : "false"));*/

  Logger::getInstance().Log(
      LogLevel::Debug, std::string("_Skybox valid = ") +
                           (bgfx::isValid(_skyProgram) ? "true" : "false"));
  _terrainShadowProgram = ShaderManager.LoadProgram(
      "terrain_shadow", "vs_shadow.bin", "fs_shadow.bin");
  Logger::getInstance().Log(
      LogLevel::Debug,
      std::string("_terrainShadowProgram valid = ") +
          (bgfx::isValid(_terrainShadowProgram) ? "true" : "false"));

  Logger::getInstance().Log(
      LogLevel::Info,
      "GameTest::Init: Loading water shaders: vs_water.bin, fs_water.bin");
  _waterProgram =
      ShaderManager.LoadProgram("water", "vs_water.bin", "fs_water.bin");

  // Create uniform handles
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
      bgfx::createUniform("u_lightMatrix", bgfx::UniformType::Mat4);
  _sunDirUniform =
      bgfx::createUniform("u_sunDirection", bgfx::UniformType::Vec4);
  _sunLumUniform =
      bgfx::createUniform("u_sunLuminance", bgfx::UniformType::Vec4);
  _skyAmbientUniform =
      bgfx::createUniform("u_skyAmbient", bgfx::UniformType::Vec4);
  _paramsUniform = bgfx::createUniform(
      "u_parameters", bgfx::UniformType::Vec4);  // Skybox parameters
  _scatterParamsUniform =
      bgfx::createUniform("u_scatterParams", bgfx::UniformType::Vec4);
  _betaRUniform = bgfx::createUniform("u_betaR", bgfx::UniformType::Vec4);
  _betaMUniform = bgfx::createUniform("u_betaM", bgfx::UniformType::Vec4);
  _viewInvUniform = bgfx::createUniform(
      "u_viewInv", bgfx::UniformType::Mat4);  // Inverse view matrix for skybox
  _projInvUniform = bgfx::createUniform(
      "u_projInv",
      bgfx::UniformType::Mat4);  // Inverse projection matrix for skybox
  // Water uniforms
  _u_waterColor = bgfx::createUniform("u_waterColor", bgfx::UniformType::Vec4);
  _s_waterTexUniform =
      bgfx::createUniform("s_waterTex", bgfx::UniformType::Sampler);
  _s_waterNormUniform =
      bgfx::createUniform("s_waterNorm", bgfx::UniformType::Sampler);
  _s_reflectionUniform =
      bgfx::createUniform("s_reflection", bgfx::UniformType::Sampler);

  // Grass uniforms
  _s_grassDiffuseUniform =
      bgfx::createUniform("s_grassDiffuse", bgfx::UniformType::Sampler);
  _s_grassORMUniform =
      bgfx::createUniform("s_grassORM", bgfx::UniformType::Sampler);
  _s_grassNormalUniform =
      bgfx::createUniform("s_grassNormal", bgfx::UniformType::Sampler);
  _u_slopeBlendParamsUniform =
      bgfx::createUniform("u_slopeBlendParams", bgfx::UniformType::Vec4);

  // Timer uniform
  _timeUniform = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);

  if (bgfx::isValid(_waterProgram)) {
    Logger::getInstance().Log(LogLevel::Info,
                              "Water program (_waterProgram) "
                              "loaded successfully. Handle: " +
                                  std::to_string(_waterProgram.idx));
  } else {
    Logger::getInstance().Log(LogLevel::Error,
                              "FAILED to load water program "
                              "(_waterProgram). Handle is invalid.");
  }

  // Generate initial heightmap data
  /* const uint16_t hm_sz = TerrainSize;
  std::vector<float> rawHeights(hm_sz * hm_sz);
  float minRawHeight = std::numeric_limits<float>::max();
  float maxRawHeight = std::numeric_limits<float>::lowest();*/

  // --- Parameters to Tune ---
  float dune_freq_x =
      20.0f;  // Increased: More waves, potentially sharper peaks
  float dune_freq_y = 17.0f;  // Increased: More waves
  float dune_base_amplitude =
      80.0f;  // Kept high: for significant raw variation
  float falloff_strength =
      0.8f;  // Increased: Falloff is a bit quicker, which can help define
             // central features more sharply and prevent distant, broad
             // plateaus if frequencies are low. If hills are too small/central,
             // reduce this (e.g., 0.4-0.6)
  /* float noise_amplitude = 1.5f;  // Kept relatively small
  float exaggeration = 30.0f;    // Kept high

  for (uint16_t y_coord = 0; y_coord < hm_sz; ++y_coord) {
    for (uint16_t x_coord = 0; x_coord < hm_sz; ++x_coord) {
      float nx =
          (static_cast<float>(x_coord) - static_cast<float>(hm_sz) * 0.5f) /
          (static_cast<float>(hm_sz) * 0.5f);
      float ny =
          (static_cast<float>(y_coord) - static_cast<float>(hm_sz) * 0.5f) /
          (static_cast<float>(hm_sz) * 0.5f);

      float r_sq = nx * nx + ny * ny;  // Square of distance from center
      float falloff = expf(-falloff_strength * r_sq);

      // Base dune shape
      float dune_shape = sinf(nx * dune_freq_x) * cosf(ny * dune_freq_y);
      float dune = dune_shape * dune_base_amplitude * falloff;

      float noise_val =
          (static_cast<float>(rand() % 1000) / 999.0f - 0.5f) * noise_amplitude;

      float currentHeight = dune + noise_val;
      currentHeight *= exaggeration;

      rawHeights[y_coord * hm_sz + x_coord] = currentHeight;
      if (currentHeight < minRawHeight)
        minRawHeight = currentHeight;
      if (currentHeight > maxRawHeight)
        maxRawHeight = currentHeight;
    }
  }

  // Normalize heights to [-1, 1] range
  std::vector<uint32_t> heightmapData(hm_sz * hm_sz);
  float overallRange = maxRawHeight - minRawHeight;

  for (uint16_t i = 0; i < hm_sz * hm_sz; ++i) {
    float normalizedHeight;
    if (overallRange >
        0.0001f) {  // Check to prevent division by zero if map is flat
      // Normalize to [0, 1] first
      normalizedHeight = (rawHeights[i] - minRawHeight) / overallRange;
      // Then map to [-1, 1]
      normalizedHeight = normalizedHeight * 2.0f - 1.0f;
    } else {
      // If the map is flat (minRawHeight is very close to maxRawHeight)
      normalizedHeight = 0.0f;  // Represents the middle height
    }

    // Final clamp just in case of minute floating point errors pushing it
    // outside [-1,1]
    normalizedHeight = bx::clamp(normalizedHeight, -1.0f, 1.0f);

    uint8_t encodedHeight =
        static_cast<uint8_t>(127.5f + 127.5f * normalizedHeight);
    uint32_t rgba =
        (encodedHeight << 24) | (0 << 16) | (0 << 8) | (encodedHeight);
    heightmapData[i] = rgba;
  }

  // Create RGBA8 texture for heightmap
  _heightTexture = bgfx::createTexture2D(
      hm_sz, hm_sz, false, 1, bgfx::TextureFormat::RGBA8,
      BGFX_TEXTURE_NONE | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
      bgfx::copy(heightmapData.data(),
                 heightmapData.size() * sizeof(uint32_t))); */

  // Create water mesh data
  // TODO: mesh data should not be here, that is handled in mesh_data.cpp/.h
  std::vector<PosTexCoord0Vertex> waterVertices;
  std::vector<uint16_t> waterIndices;

  /* for (uint16_t y = 0; y < TerrainSize; ++y) {
    for (uint16_t x = 0; x < TerrainSize; ++x) {
      float u = float(x) / (TerrainSize - 1);
      float v = float(y) / (TerrainSize - 1);
      waterVertices.push_back({(float)x, 0.0f, (float)y, u, v});
    }
  } */

  // Create waterIndices using same grid method as terrain
  /* const uint16_t gridSize = TerrainSize;
  for (uint16_t y = 0; y < gridSize - 1; ++y) {
    for (uint16_t x = 0; x < gridSize - 1; ++x) {
      uint16_t i = y * gridSize + x;

      waterIndices.push_back(i);
      waterIndices.push_back(i + 1);
      waterIndices.push_back(i + gridSize);

      waterIndices.push_back(i + 1);
      waterIndices.push_back(i + gridSize + 1);
      waterIndices.push_back(i + gridSize);
    }
  }*/

  // Generate terrain mesh (grid)
  /* terrainVertices.clear();
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
      terrainIndices.push_back(i + 1);
      terrainIndices.push_back(i + TerrainSize);
      terrainIndices.push_back(i + 1);
      terrainIndices.push_back(i + TerrainSize + 1);
      terrainIndices.push_back(i + TerrainSize);
    }
  } */

  // Create terrain vertex and index buffers
  /* _terrainVbh = bgfx::createVertexBuffer(
      bgfx::copy(terrainVertices.data(),
                 terrainVertices.size() * sizeof(PosTexCoord0Vertex)),
      PosTexCoord0Vertex::layout);
  _terrainIbh = bgfx::createIndexBuffer(bgfx::copy(
      terrainIndices.data(), terrainIndices.size() * sizeof(uint16_t)));*/

  // Create shadow vertex buffer
  std::vector<ShadowVertex> shadowVertices;
  for (const auto& v : terrainVertices) {
    shadowVertices.push_back({v.x, v.y, v.z, v.u, v.v});
  }
  _shadowVbh = bgfx::createVertexBuffer(
      bgfx::copy(shadowVertices.data(),
                 shadowVertices.size() * sizeof(ShadowVertex)),
      ShadowVertex::layout);

  // Create water vertex buffer
  _waterVbh = bgfx::createVertexBuffer(
      bgfx::copy(waterVertices.data(),
                 waterVertices.size() * sizeof(PosTexCoord0Vertex)),
      PosTexCoord0Vertex::layout);
  _waterIbh = bgfx::createIndexBuffer(
      bgfx::copy(waterIndices.data(), waterIndices.size() * sizeof(uint16_t)));

  // Create shadow map texture and framebuffer
  shadowMapTexture = bgfx::createTexture2D(
      KNOWN_SHADOW_MAP_SIZE, KNOWN_SHADOW_MAP_SIZE, false, 1,
      bgfx::TextureFormat::D32, BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_LESS);
  shadowMapFB = bgfx::createFrameBuffer(1, &shadowMapTexture, true);

  // Setup camera
  // TODO: use camera component here
  /* float terrainCenterPos[3] = {TerrainExtent, 0.0f, TerrainExtent};
  float targetPos[3] = {
      TerrainExtent, 0.0f,
      TerrainExtent};  // Define the camera's initial target position */

  /* camera.setTarget(targetPos);  // Set the camera to look at the target
  position camera.setDistance(TerrainScale * 2.0f);  // Set initial camera
  distance camera.setPitch(bx::toRad(30.0f));        // Look downward
  camera.setYaw(bx::toRad(45.0f));          // Set initial yaw
  */

  // Initialize terrain world matrix (scale)
  /* bx::mtxIdentity(this->world_matrix);
  float terrainCenterOffset = (TerrainSize - 1) * 0.5f;
  float scaleMtx[16];
  float translateMtx[16];
  bx::mtxScale(scaleMtx, TerrainScale, 10.0f, TerrainScale);
  float desiredTerrainCenter[3] = {32.0f, -147.826f, 200.0f};
  bx::mtxTranslate(
      translateMtx,
      desiredTerrainCenter[0] - terrainCenterOffset * TerrainScale,
      desiredTerrainCenter[1],
      desiredTerrainCenter[2] - terrainCenterOffset * TerrainScale);
  bx::mtxMul(this->world_matrix, scaleMtx, translateMtx);

  // Initialize water model matrix (flat plane)
  // Bring in the water matrix member

  bx::mtxIdentity(this->waterModelMatrix);
  bx::mtxScale(scaleMtx, TerrainScale, 1.0f, TerrainScale);
  bx::mtxTranslate(
      translateMtx,
      desiredTerrainCenter[0] - terrainCenterOffset * TerrainScale,
      desiredTerrainCenter[1] + 0.1f,  // Slight Y offset to sit above terrain
      desiredTerrainCenter[2] - terrainCenterOffset * TerrainScale);
  bx::mtxMul(this->waterModelMatrix, scaleMtx, translateMtx);*/
}

// ═══════════════════════════════════════════════════════════════
// CREATE TERRAIN ENTITY
// ═══════════════════════════════════════════════════════════════
void GameTest::GenerateTerrain(const PCG::Config& gen_config,
                               uint16_t gridSize) {

  // Generate mesh
  PCG::Generator gen(gen_config);
  PCG::Generator::MeshOutput mesh_params;
  mesh_params.size = gridSize;      // GRID SIZE
  mesh_params.with_normals = true;  // ENABLE NORMALS
  mesh_params.with_colors = true;

  std::cout << "GenerateTerrain called with "
            << gen_config.constraints.GetRuleCount() << " rules" << std::endl;

  auto mesh_data = gen.GenerateMesh(mesh_params);
  terrain_mesh_ = std::make_shared<Mesh>(mesh_data);

  // Create ECS entity
  auto& world = ECS::Main();

  // Check if terrain exists
  if (terrain_entity_ == entt::null) {
    auto [entity, transform] = world.CreateEntity("Terrain");

    world.Add<MeshRendererComponent>(entity, terrain_mesh_.get(), true);

    Transform::SetPosition(transform, glm::vec3(0.0f, 0.0f, 0.0f),
                           Space::LOCAL);
    Transform::SetScale(transform, glm::vec3(2.0f),
                        Space::LOCAL);  // WORLD SCALE
    Transform::Evaluate(transform);

    terrain_entity_ = entity;
    Logger::getInstance().Log(LogLevel::Info, "Terrain entity created.");

  } else {
    // Update terrain mesh
    auto& renderer = world.Get<MeshRendererComponent>(terrain_entity_);
    renderer.mesh_ = terrain_mesh_.get();
    Logger::getInstance().Log(LogLevel::Info, "Terrain mesh updated.");
  }
  Logger::getInstance().Log(
      LogLevel::Info, "Terrain entity created: " +
                          std::to_string(mesh_data.positions_.size() / 3) +
                          " verts");
}

// Updates game state per frame
void GameTest::Update() {
  // Update camera from keyboard input
  // cameraSystem.UpdateFromKeyboard();

  float dt = bx::kPi * 0.002f;  // water time step
  _waterTime += dt;
}

// Renders the game scene
void GameTest::Render() {
  // Ensure essential shader programs are valid before attempting to render
  /* if (!bgfx::isValid(_terrainProgramHeight) || !bgfx::isValid(_waterProgram)
  || !bgfx::isValid(_terrainShadowProgram)) {
    Logger::getInstance().Log(LogLevel::Error,
                              "GameTest::Render: One or more game programs "
                              "invalid. Aborting Render.");
    return;
  }*/

  // --- Shadow Map Pass ---
  /* bgfx::setViewFrameBuffer(ViewID::SHADOW_PASS,
                           shadowMapFB);  // Set render target to shadow map
  bgfx::setViewRect(ViewID::SHADOW_PASS, 0, 0, KNOWN_SHADOW_MAP_SIZE,
                    KNOWN_SHADOW_MAP_SIZE);  // Set viewport for shadow map
  bgfx::setViewClear(ViewID::SHADOW_PASS, BGFX_CLEAR_DEPTH, 0x000000FF, 1.0f,
                     0);
  // Get the local grid center point in local terrain space
  float localGridCenterX = (TerrainSize - 1) * 0.5f;
  float localGridCenterZ = (TerrainSize - 1) * 0.5f;
  float localGridCenterY = 0.0f;

  // Transform this local center point by the terrain's FULL world matrix
  float localCenterHomogeneous[4] = {localGridCenterX, localGridCenterY,
                                     localGridCenterZ, 1.0f};
  float worldCenterHomogeneous[4];
  bx::vec4MulMtx(worldCenterHomogeneous, localCenterHomogeneous,
                 this->world_matrix);
  // This is now our NEW actualLightTarget in world space
  bx::Vec3 actualLightTarget_WorldCorrected = {
      worldCenterHomogeneous[0],
      worldCenterHomogeneous[1] +
          TERRAIN_MAX_ACTUAL_HEIGHT * 0.25f,  // Add some height offset
      worldCenterHomogeneous[2]}; */

  // Setup light view and projection matrices for shadow mapping
  // TODO: make sure to handle sun direction based on skybox
  /* float lightViewMatrix[16], lightProjMatrix[16];
  float terrainWorldSize = (TerrainSize - 1) * TerrainScale;
  bx::Vec3 actualLightTarget = actualLightTarget_WorldCorrected;
  bx::Vec3 lightPos = bx::mad(sunDirectionVec, -terrainWorldSize * 1.5f,
                              actualLightTarget);  // Offset from the new
  target bx::mtxLookAt(lightViewMatrix, lightPos, actualLightTarget,
                bx::Vec3(0.0f, 1.0f, 0.0f));

  float orthoHalfSize = terrainWorldSize * 0.6f;
  bx::mtxOrtho(lightProjMatrix, -orthoHalfSize, orthoHalfSize, -orthoHalfSize,
               orthoHalfSize, 0.1f, terrainWorldSize * 3.0f, 0.0f,
               bgfx::getCaps()->homogeneousDepth);
  bgfx::setViewTransform(ViewID::SHADOW_PASS, lightViewMatrix,
  lightProjMatrix); bgfx::setTransform(this->world_matrix);

  float terrainParamsArr[4] = {TERRAIN_MAX_ACTUAL_HEIGHT, 0.0f, TerrainScale,
                               TerrainScale};
  bgfx::setUniform(_terrainParamsUniform, terrainParamsArr);

  bgfx::setTexture(3, _heightUniform, _heightTexture);
  bgfx::setTransform(this->world_matrix);
  bgfx::setVertexBuffer(0, _shadowVbh);
  bgfx::setIndexBuffer(_terrainIbh);
  bgfx::setState(BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS |
                 BGFX_STATE_CULL_CW);
  bgfx::submit(ViewID::SHADOW_PASS, _terrainShadowProgram);*/

  // --- Reflection Pass ---

  // Allow access to the renderer subsystem
  // Get the GraphicsRenderer instance
  /* GraphicsRenderer* graphicsRendererPtr = nullptr;  // Initialize to
  nullptr auto* baseRendererPtr = Runtime::Renderer();

  if (baseRendererPtr) {
    graphicsRendererPtr = dynamic_cast<GraphicsRenderer*>(baseRendererPtr);
  }

  if (!graphicsRendererPtr) {
    Logger::getInstance().Log(LogLevel::Error,
                              "GameTest::Render: Failed to get "
                              "GraphicsRenderer instance. Aborting Render.");
    return;
  }
  if (bgfx::isValid(graphicsRendererPtr->GetReflectionFramebuffer()) &&
      bgfx::isValid(graphicsRendererPtr->GetReflectionColorTex())) {

    uint16_t fbw = graphicsRendererPtr->GetFramebufferWidth();
    uint16_t fbh = graphicsRendererPtr->GetFramebufferHeight();

    bgfx::setViewRect(ViewID::REFLECTION_PASS, 0, 0, fbw, fbh);
    bgfx::setViewFrameBuffer(ViewID::REFLECTION_PASS,
                             graphicsRendererPtr->GetReflectionFramebuffer());

    float viewMatrix[16], projMatrix[16];
    camera.getViewMatrix(viewMatrix);
    camera.getProjectionMatrix(projMatrix, float(fbw) / float(fbh));

    // Defined here for reflection pass, we cannot reuse the terrain's one
    float cameraView[16];
    // Get view matrix with translation
    camera.getViewMatrix(viewMatrix);

    // Construct reflection matrix
    float reflectMat[16];
    bx::mtxIdentity(reflectMat);
    reflectMat[5] = -1.0f;
    reflectMat[13] = 1.0f * waterModelMatrix[13];  // reflection plane Y

    // Create reflected view matrix (WITH camera translation � for terrain)
    float reflectedView[16];
    bx::mtxMul(reflectedView, viewMatrix, reflectMat);

    // Create *rotation-only* version for skybox
    float viewRotOnly[16];
    memcpy(viewRotOnly, reflectedView, sizeof(viewRotOnly));
    viewRotOnly[12] = viewRotOnly[13] = viewRotOnly[14] =
        0.0f;  // remove translation
  } */

  // --- Main Scene Pass ---
  /* bgfx::setViewRect(ViewID::SCENE_SKYBOX, 0, 0, canvasViewportW,
                    canvasViewportH);
  bgfx::setViewClear(ViewID::SCENE_SKYBOX, BGFX_CLEAR_COLOR |
  BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

  // Calculate inverse view and projection matrices for skybox
  float invViewMatrix[16], invProjMatrix[16];
  bx::mtxInverse(invViewMatrix, viewMatrix);
  bx::mtxInverse(invProjMatrix, projMatrix);

  // --- Terrain Pass ---
  constexpr uint8_t terrainViewID = ViewID::SCENE_TERRAIN;
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

  bx::mtxMul(finalLightMatrixForShader, biasMatrix, lightVPMatrix);
  bgfx::setUniform(_lightMatrixUniform, finalLightMatrixForShader);

  bgfx::setUniform(_terrainParamsUniform, terrainParamsArr);
  float hmTexelSize[4] = {1.0f / TerrainSize, 1.0f / TerrainSize, 0.0f, 0.0f};
  bgfx::setUniform(_heightmapTexelSizeUniform, hmTexelSize);

  // Slope blending parameters:
  // x = dot product value where grass starts to appear (e.g., 0.7 for ~45
  // degrees from vertical) y = dot product value where grass is fully opaque
  // (e.g., 0.9 for ~25 degrees from vertical) The dot product is between the
  // surface normal and the world up vector (0,1,0).

  float slopeParams[4] = {
      -0.5f, 0.5f, 0.0f,
      0.0f};  // Grass on anything from slightly overhanging up to 60 deg
  bgfx::setUniform(_u_slopeBlendParamsUniform, slopeParams);

  bgfx::setTexture(0, _s_diffuseUniform, terrainDiffuse);
  bgfx::setTexture(1, _s_ormUniform, terrainORM);
  bgfx::setTexture(2, _s_normalUniform, terrainNormal);
  bgfx::setTexture(3, _heightUniform, _heightTexture);
  bgfx::setTexture(4, _shadowSamplerUniform, shadowMapTexture);
  bgfx::setTexture(9, _s_grassDiffuseUniform, grassDiffuse);
  bgfx::setTexture(10, _s_grassORMUniform, grassORM);
  bgfx::setTexture(11, _s_grassNormalUniform, grassNormal);

  bgfx::setVertexBuffer(0, _terrainVbh);
  bgfx::setIndexBuffer(_terrainIbh);
  bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_CULL_CW | BGFX_STATE_MSAA);
  bgfx::submit(terrainViewID, _terrainProgramHeight); */

  // TODO: this should be removed later
  /* auto& world = ECS::Main();
  TransformSystem::Perform(viewProjection);  // update matrices
  const auto& rq = world.GetRenderQueue();

  bgfx::setViewTransform(ViewID::SCENE_FORWARD, viewMatrix, projMatrix);

  for (auto& [entity, tr, mr] : rq) {
    if (!mr.enabled_ || !mr.mesh_)
      continue;

    bgfx::setTransform(&tr.model_[0][0]);
    mr.mesh_->Bind();

    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_Z |
                   BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW);

    bgfx::submit(ViewID::SCENE_FORWARD, _terrainProgramHeight);
  }
  */

  // --- Water Pass ---
  /* bgfx::setViewClear(ViewID::WATER_PASS, BGFX_CLEAR_NONE, 0, 1.0f, 0);
  bgfx::setViewRect(ViewID::WATER_PASS, 0, 0, canvasViewportW,
  canvasViewportH); bgfx::setViewTransform(ViewID::WATER_PASS, viewMatrix,
  projMatrix);

  bgfx::setTransform(waterModelMatrix);  // Flat plane or positioned
  float timeVec[4] = {_waterTime, 0.0f, 0.0f, 0.0f};
  float waterColorVec[4] = {0.2f, 0.4f, 0.5f, 1.0f};

  // Set uniforms for water shader
  bgfx::setUniform(_cameraPosUniform, camPos);
  bgfx::setUniform(_sunDirUniform, sunDirShader);
  bgfx::setUniform(_sunLumUniform, _sunColorArray);
  bgfx::setUniform(_skyAmbientUniform, _skyAmbientArray);
  bgfx::setUniform(_timeUniform, timeVec);
  bgfx::setUniform(_u_waterColor, waterColorVec);

  bgfx::setTexture(5, _s_waterTexUniform, _waterTex);
  bgfx::setTexture(6, _s_waterNormUniform, _waterNormalTex);
  bgfx::setTexture(8, _s_reflectionUniform,
                   graphicsRendererPtr->GetReflectionColorTex());

  bgfx::setVertexBuffer(0, _waterVbh);
  bgfx::setIndexBuffer(_waterIbh);
  const uint64_t WATER_STATE = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                               BGFX_STATE_MSAA | BGFX_STATE_BLEND_ALPHA |
                               BGFX_STATE_DEPTH_TEST_LEQUAL;  // no
  DEPTH_WRITE

  bgfx::setState(WATER_STATE);

  bgfx::submit(ViewID::WATER_PASS, _waterProgram); */
}

// Releases all BGFX resources
void GameTest::Destroy() {
  Logger::getInstance().Log(LogLevel::Debug, "GameTest: Destroy() called.");

  // Lambda helper to destroy a BGFX handle if valid
  auto destroyHandle = [](auto& handle) {
    if (bgfx::isValid(handle)) {
      bgfx::destroy(handle);
      handle = BGFX_INVALID_HANDLE;  // Reset handle to invalid
    }
  };

  // Destroy shader programs
  destroyHandle(_terrainProgramHeight);
  destroyHandle(_skyProgram);
  destroyHandle(_terrainShadowProgram);
  destroyHandle(_waterProgram);

  // Destroy textures
  destroyHandle(_heightTexture);
  destroyHandle(terrainDiffuse);
  destroyHandle(terrainORM);
  destroyHandle(terrainNormal);
  destroyHandle(shadowMapTexture);
  destroyHandle(grassDiffuse);
  destroyHandle(grassORM);
  destroyHandle(grassNormal);

  // Destroy framebuffers
  destroyHandle(shadowMapFB);

  // Destroy vertex and index buffers
  destroyHandle(_terrainVbh);
  destroyHandle(_terrainIbh);
  destroyHandle(_skyVbh);
  destroyHandle(_skyIbh);
  destroyHandle(_shadowVbh);
  destroyHandle(_waterVbh);
  destroyHandle(_waterIbh);

  // Destroy uniform handles
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
  destroyHandle(_timeUniform);
  destroyHandle(_scatterParamsUniform);
  destroyHandle(_betaRUniform);
  destroyHandle(_betaMUniform);
  destroyHandle(_s_grassDiffuseUniform);
  destroyHandle(_s_grassORMUniform);
  destroyHandle(_s_grassNormalUniform);
  destroyHandle(_u_slopeBlendParamsUniform);

  // Destroy water uniforms
  destroyHandle(_u_waterColor);
  destroyHandle(_s_waterTexUniform);
  destroyHandle(_s_waterNormUniform);
  destroyHandle(_s_reflectionUniform);
  destroyHandle(_waterTex);
  destroyHandle(_waterNormalTex);

  Logger::getInstance().Log(LogLevel::Debug, "GameTest: Destroy() completed");
}