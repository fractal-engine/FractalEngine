#include "game/game_test.h"

#include <SDL.h>
#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <string.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>  // TODO: remove this?
#include <vector>

#include "editor/runtime/runtime.h"
#include "engine/context/engine_context.h"
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

// REMOVE THESE - only used for procmodel
#include "engine/content/loaders/mesh_loader.h"
#include "engine/pcg/procmodel/descriptor/descriptor_builder.h"
#include "engine/pcg/procmodel/model_graph/model_graph_builder.h"

/*******************************************************************************
 * TODO:
 * - Move bgfx uniforms to graphics renderer?
 * - Move load terrain textures to runtime.cpp (?)
 ******************************************************************************/

// ──────────────────────────────────────────────────────
//  Vertex layouts
// ──────────────────────────────────────────────────────
bgfx::VertexLayout PosTexCoord0Vertex::layout;

// Maximum height of the terrain in world units
// constexpr float TERRAIN_MAX_ACTUAL_HEIGHT = 150.0f;
// View ID for the shadow map rendering pass
// constexpr uint8_t SHADOW_MAP_VIEW_ID = ViewID::Shadow(0);
// Fixed size for the shadow map texture
// constexpr uint16_t KNOWN_SHADOW_MAP_SIZE = 2048;

// Vertex structure for screen-space positions (e.g., full-screen quad)
struct ScreenPosVertex {
  float x, y;
  static bgfx::VertexLayout layout;
};

bgfx::VertexLayout
    ScreenPosVertex::layout;  // Definition for ScreenPosVertex layout

// Vertex structure for shadow map rendering (position and UVs for heightmap
// lookup)
/* struct ShadowVertex {
  float x, y, z;
  float u, v;
  static bgfx::VertexLayout layout;
};
bgfx::VertexLayout ShadowVertex::layout;  // Definition for ShadowVertex layout
*/

// Initializes static vertex layouts
void PosTexCoord0Vertex::init() {
  // Initialize layout for PosTexCoord0Vertex (position and texture coordinates)
  /* PosTexCoord0Vertex::layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .end();*/

  // Initialize layout for ShadowVertex (position and texture coordinates for
  // shadow map)
  /* ShadowVertex::layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .end(); */
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

  // TODO: mesh data should not be here, that is handled in mesh_data.cpp/.h

  // Create shadow vertex buffer
  /* std::vector<ShadowVertex> shadowVertices;
  for (const auto& v : terrainVertices) {
    shadowVertices.push_back({v.x, v.y, v.z, v.u, v.v});
  }
  _shadowVbh = bgfx::createVertexBuffer(
      bgfx::copy(shadowVertices.data(),
                 shadowVertices.size() * sizeof(ShadowVertex)),
      ShadowVertex::layout);*/

  // Create shadow map texture and framebuffer
  /* shadowMapTexture = bgfx::createTexture2D(
      KNOWN_SHADOW_MAP_SIZE, KNOWN_SHADOW_MAP_SIZE, false, 1,
      bgfx::TextureFormat::D32, BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_LESS);
  shadowMapFB = bgfx::createFrameBuffer(1, &shadowMapTexture, true); */

  // ── Procmodel vertical slice test ──
  auto& pcg = EngineContext::Generator();
  auto result = pcg.RequestInstance(
      "/Users/louismercier/Projects/FractalEngine/build/macosx/x86_64/release/"
      "examples/example-project/test_model.json",
      7643);

  if (result.root != entt::null) {
    Logger::getInstance().Log(LogLevel::Info,
                              "[GameTest] Procmodel test: spawned " +
                                  std::to_string(result.part_entities.size()) +
                                  " parts");
  } else {
    Logger::getInstance().Log(LogLevel::Error,
                              "[GameTest] Procmodel test: failed to spawn");
  }

  // DEBUG - display file hierarchy in logger
  auto scene = Content::MeshLoader::LoadScene(
      "/Users/louismercier/Projects/FractalEngine/build/macosx/x86_64/release/"
      "examples/example-project/test_model.glb");

  auto graph = ProcModel::ModelGraphBuilder::Build(scene, "test_model.glb");

  auto desc = ProcModel::DescriptorBuilder::Build("test_model",
                                                  "test_model.glb", graph);

  for (const auto& group : desc.selection_groups) {
    Logger::getInstance().Log(
        LogLevel::Info, "[Test] Group: " + group.group_id + " (" +
                            std::to_string(group.parts.size()) + " parts)");
    for (const auto& part : group.parts) {
      Logger::getInstance().Log(LogLevel::Info, "  - " + part.id);
    }
  }
}

// ═══════════════════════════════════════════════════════════════
// CREATE TERRAIN ENTITY
// ═══════════════════════════════════════════════════════════════

/* ---------------
void GameTest::GenerateTerrain(const PCG::Config& gen_config,
                               uint16_t gridSize) {
/*
  // Generate mesh
  PCG::Generator gen(gen_config);
  PCG::Generator::MeshOutput mesh_params;
  mesh_params.size = gridSize;      // GRID SIZE
  mesh_params.with_normals = true;  // ENABLE NORMALS
  mesh_params.with_colors = true;

  std::cout << "GenerateTerrain called with "
            << gen_config.constraints.GetRuleCount() << " rules" << std::endl;

  // auto mesh_data = gen.GenerateMesh(mesh_params);
  // terrain_mesh_ = std::make_shared<Mesh>(mesh_data);

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
  Logger::getInstance().Log(LogLevel::Info,
                            "Terrain entity created: " +
                                std::to_string(mesh_data.positions.size() / 3) +
                                " verts");
} --------------- */

// Updates game state per frame
void GameTest::Update() {
  // Update camera from keyboard input
  // cameraSystem.UpdateFromKeyboard();

  // float dt = bx::kPi * 0.002f;  // water time step
  // _waterTime += dt;
}

// Renders the game scene
void GameTest::Render() {}

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
