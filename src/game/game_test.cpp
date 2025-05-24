#include "game/game_test.h"

#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <vector>

#include "core/logger.h"
#include "core/view_ids.h"
#include "renderer/shaders/shader_utils.h"
#include "subsystem/subsystem_manager.h"
#include "tools/texture_utils.h"

// --- Local Math Workarounds  ---
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

// --- Vertex Structures and Layouts ---
// Vertex structure for position and one set of texture coordinates
bgfx::VertexLayout PosTexCoord0Vertex::layout;

// Maximum height of the terrain in world units
constexpr float TERRAIN_MAX_ACTUAL_HEIGHT = 150.0f;
// View ID for the shadow map rendering pass
constexpr uint8_t SHADOW_MAP_VIEW_ID = ViewID::SHADOW_PASS;
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
  bx::mtxIdentity(world_matrix);  // Initialize world matrix to identity
  // Default sky ambient color (dark)
  _skyAmbientArray[0] = 0.1f;
  _skyAmbientArray[1] = 0.1f;
  _skyAmbientArray[2] = 0.1f;
  _skyAmbientArray[3] = 0.0f;  // w component unused for color
}

// Destructor: Cleanup is handled in Shutdown()
GameTest::~GameTest() {}

// Initializes game resources
void GameTest::Init() {
  Logger::getInstance().Log(LogLevel::Debug, "[GameTest] Init() called.");

  // Initialize vertex layouts
  PosTexCoord0Vertex::init();
  if (!ScreenPosVertex::layout.m_stride) {  // Initialize ScreenPosVertex layout
                                            // if not already done
    ScreenPosVertex::layout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .end();
  }

  // Setup camera

  float terrainCenterPos[3] = {TerrainExtent, 0.0f, TerrainExtent};
  float targetPos[3] = {
      TerrainExtent, 0.0f,
      TerrainExtent};           // Define the camera's initial target position
  camera.setTarget(targetPos);  // Set the camera to look at the target position
  camera.setDistance(TerrainScale * 2.0f);  // Set initial camera distance
  camera.setPitch(bx::toRad(30.0f));        // Look downward
  camera.setYaw(bx::toRad(45.0f));          // Set initial yaw

  // Load shader programs
  auto& shaderMgr = *SubsystemManager::GetShaderManager();
  _terrainProgramHeight =
      shaderMgr.LoadProgram("terrain_pbr", "vs_terrain.bin", "fs_terrain.bin");
  Logger::getInstance().Log(
      LogLevel::Debug,
      std::string("[DEBUG] _terrainPBR valid = ") +
          (bgfx::isValid(_terrainProgramHeight) ? "true" : "false"));
  _skyProgram =
      shaderMgr.LoadProgram("skybox_proc", "vs_skybox.bin", "fs_skybox.bin");
  Logger::getInstance().Log(
      LogLevel::Debug, std::string("[DEBUG] _Skybox valid = ") +
                           (bgfx::isValid(_skyProgram) ? "true" : "false"));
  _terrainShadowProgram =
      shaderMgr.LoadProgram("terrain_shadow", "vs_shadow.bin", "fs_shadow.bin");
  Logger::getInstance().Log(
      LogLevel::Debug,
      std::string("[DEBUG] _terrainShadowProgram valid = ") +
          (bgfx::isValid(_terrainShadowProgram) ? "true" : "false"));

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
  _viewInvUniform = bgfx::createUniform(
      "u_viewInv", bgfx::UniformType::Mat4);  // Inverse view matrix for skybox
  _projInvUniform = bgfx::createUniform(
      "u_projInv",
      bgfx::UniformType::Mat4);  // Inverse projection matrix for skybox

  // Load terrain textures
  terrainDiffuse =
      TextureUtils::LoadTexture("assets/textures/terrain/basecolor.tga");
  terrainORM = TextureUtils::LoadTexture("assets/textures/terrain/ORM.tga");
  terrainNormal =
      TextureUtils::LoadTexture("assets/textures/terrain/Normal.tga");

  // Generate initial heightmap data
  const uint16_t hm_sz = TerrainSize;
  std::vector<uint8_t> heightmapData(hm_sz * hm_sz);
  for (uint16_t y = 0; y < hm_sz; ++y) {
    for (uint16_t x = 0; x < hm_sz; ++x) {
      float nx = (x - hm_sz * 0.5f) / (hm_sz * 0.5f);  // [-1, 1]
      float ny = (y - hm_sz * 0.5f) / (hm_sz * 0.5f);  // [-1, 1]
      float dist = sqrtf(nx * nx + ny * ny);

      // Flat center plateau with cliff walls
      float cliff = local_smoothStep(0.35f, 0.3f, dist);
      float height = cliff * 0.8f;  // Raise plateau

      // Add ripples
      float ripple = sinf(nx * 12.0f) * cosf(ny * 12.0f);
      height += ripple * 0.1f;

      // Erosion pattern
      float erosion = sinf((nx + ny) * 4.0f);
      height -= erosion * 0.05f;

      // Small random noise
      float noise = ((rand() % 1000) / 1000.0f - 0.5f) * 0.02f;
      height += noise;

      height = bx::clamp(height, -1.0f, 1.0f);
      heightmapData[y * hm_sz + x] = uint8_t(127 + 127 * height);
    }
  }

  // Create heightmap texture
  _heightTexture = bgfx::createTexture2D(
      hm_sz, hm_sz, false, 1, bgfx::TextureFormat::R8,
      BGFX_TEXTURE_NONE | BGFX_SAMPLER_U_CLAMP |
          BGFX_SAMPLER_V_CLAMP,  // Clamp sampling for heightmap
      bgfx::copy(heightmapData.data(), heightmapData.size()));

  // Generate terrain mesh (grid)
  terrainVertices.clear();
  terrainIndices.clear();
  for (uint16_t y = 0; y < TerrainSize; ++y) {
    for (uint16_t x = 0; x < TerrainSize; ++x) {
      terrainVertices.push_back(
          {(float)x, 0.0f, (float)y,  // Position (x, 0, z), y is set by shader
           (float)x / (TerrainSize - 1),
           (float)y / (TerrainSize - 1)});  // UV coordinates
    }
  }
  for (uint16_t y = 0; y < TerrainSize - 1; ++y) {
    for (uint16_t x = 0; x < TerrainSize - 1; ++x) {
      uint16_t i = y * TerrainSize + x;  // Current vertex index
      // First triangle of the quad
      terrainIndices.push_back(i);  // build order CCW
      terrainIndices.push_back(i + 1);
      terrainIndices.push_back(i + TerrainSize);

      // Second triangle of the quad
      terrainIndices.push_back(i + 1);
      terrainIndices.push_back(i + TerrainSize + 1);
      terrainIndices.push_back(i + TerrainSize);
    }
  }

  // Create terrain vertex and index buffers
  _terrainVbh = bgfx::createVertexBuffer(
      bgfx::copy(terrainVertices.data(),
                 terrainVertices.size() * sizeof(PosTexCoord0Vertex)),
      PosTexCoord0Vertex::layout);
  _terrainIbh = bgfx::createIndexBuffer(bgfx::copy(
      terrainIndices.data(), terrainIndices.size() * sizeof(uint16_t)));

  // Create shadow vertex buffer (uses same geometry as terrain but different
  // layout for shadow shader)
  std::vector<ShadowVertex> shadowVertices;
  for (const auto& v : terrainVertices) {
    shadowVertices.push_back(
        {v.x, v.y, v.z, v.u, v.v});  // Copy data to ShadowVertex format
  }
  _shadowVbh = bgfx::createVertexBuffer(
      bgfx::copy(shadowVertices.data(),
                 shadowVertices.size() * sizeof(ShadowVertex)),
      ShadowVertex::layout);

  // Create skybox geometry buffers
  createSkyboxBuffers();

  // Create shadow map texture and framebuffer
  shadowMapTexture = bgfx::createTexture2D(
      KNOWN_SHADOW_MAP_SIZE, KNOWN_SHADOW_MAP_SIZE, false, 1,
      bgfx::TextureFormat::D16,  // 16-bit depth texture
      BGFX_TEXTURE_RT_WRITE_ONLY |
          BGFX_SAMPLER_COMPARE_LESS);  // Render target, enable depth comparison
  shadowMapFB = bgfx::createFrameBuffer(1, &shadowMapTexture,
                                        true);  // Depth-only framebuffer

  // Initialize terrain world matrix (scale)
  bx::mtxIdentity(this->world_matrix);
  // Scale terrain grid vertices to world space; height (Y) is determined by
  // shader from heightmap
  // 1. Compute center offset in mesh local space
  float terrainCenterOffset = (TerrainSize - 1) * 0.5f;  // Local-space center

  // 2. Create scale and translation matrices
  float scaleMtx[16];
  float translateMtx[16];
  bx::mtxScale(scaleMtx, TerrainScale, 1.0f, TerrainScale);

  // 3. Compute the world offset that brings terrain center to desired position
  float desiredTerrainCenter[3] = {-31.683f, 0.0f, -27.185f};
  bx::mtxTranslate(
      translateMtx,
      desiredTerrainCenter[0] - terrainCenterOffset * TerrainScale,
      desiredTerrainCenter[1],
      desiredTerrainCenter[2] - terrainCenterOffset * TerrainScale);

  // 4. Combine translation and scale into world matrix
  bx::mtxMul(this->world_matrix, scaleMtx, translateMtx);
}

// Creates vertex and index buffers for the skybox (a full-screen quad)
void GameTest::createSkyboxBuffers() {
  // Quad vertices for full-screen rendering (clip space)
  static const ScreenPosVertex quadVertices[] = {
      {-1.0f, 1.0f}, {1.0f, 1.0f}, {-1.0f, -1.0f}, {1.0f, -1.0f}};
  // Quad indices
  static const uint16_t quadIndices[] = {0, 2, 1, 1, 2, 3};

  _skyVbh = bgfx::createVertexBuffer(
      bgfx::makeRef(quadVertices, sizeof(quadVertices)),
      ScreenPosVertex::layout);
  _skyIbh =
      bgfx::createIndexBuffer(bgfx::makeRef(quadIndices, sizeof(quadIndices)));
}

// Updates game state per frame
void GameTest::Update() {
  // Update camera from keyboard input
  cameraSystem.UpdateFromKeyboard();

  // Update sun angle for rendering
  _cycleTime += 0.0007f;
  if (_cycleTime > bx::kPi * 2.0f) {
    _cycleTime -= bx::kPi * 2.0f;
  }
}

// Renders the game scene
void GameTest::Render() {
  // Ensure essential shader programs are valid before attempting to render
  if (!bgfx::isValid(_terrainProgramHeight) || !bgfx::isValid(_skyProgram) ||
      !bgfx::isValid(_terrainShadowProgram)) {
    return;
  }

  // --- Camera and Lighting Setup ---
  float viewMatrix[16], projMatrix[16];
  camera.getViewMatrix(viewMatrix);  // Get current view matrix from camera
  float aspectRatio = (canvasViewportW > 0 && canvasViewportH > 0)
                          ? float(canvasViewportW) / float(canvasViewportH)
                          : 1.0f;  // Calculate aspect ratio
  camera.getProjectionMatrix(projMatrix,
                             aspectRatio);  // Get current projection matrix

  // Calculate sun direction and color based on cycle time
  float sunAngle = _cycleTime;
  bx::Vec3 sunDirectionVec =
      bx::normalize(bx::Vec3(bx::cos(sunAngle), bx::sin(sunAngle), 0.1f));
  if (sunDirectionVec.y < -0.2f)
    sunDirectionVec.y = -0.2f;  // Clamp sun minimum height
  float sunDirShader[4] = {sunDirectionVec.x, sunDirectionVec.y,
                           sunDirectionVec.z, 0.0f};

  float sunElevationFactor = local_smoothStep(
      -0.15f, 0.2f,
      sunDirectionVec.y);  // Smooth transition for sun color/intensity
  float sunIntensity = bx::lerp(
      0.3f, 3.0f, sunElevationFactor);  // Sun intensity based on elevation

  // Sun color (luminance) based on elevation and intensity
  _sunColorArray[0] = bx::lerp(0.8f, 1.0f, sunElevationFactor) * sunIntensity;
  _sunColorArray[1] = bx::lerp(0.6f, 1.0f, sunElevationFactor) * sunIntensity;
  _sunColorArray[2] = bx::lerp(0.4f, 1.0f, sunElevationFactor) * sunIntensity;
  _sunColorArray[3] = 0.0f;

  // Sky ambient color based on sun elevation
  _skyAmbientArray[0] = bx::lerp(0.02f, 0.3f, sunElevationFactor);
  _skyAmbientArray[1] = bx::lerp(0.03f, 0.4f, sunElevationFactor);
  _skyAmbientArray[2] = bx::lerp(0.05f, 0.55f, sunElevationFactor);
  _skyAmbientArray[3] = 0.0f;

  // Apply a global boost to ambient light for better visibility
  const float ambientBoost = 0.05f;
  for (int i = 0; i < 3; ++i)
    _skyAmbientArray[i] *= ambientBoost;

  // Skybox parameters (e.g., scattering coefficients, mie phase, time)
  _parametersArray[0] = 0.005f;      // Rayleigh scattering coefficient
  _parametersArray[1] = 0.1f;        // Mie scattering coefficient
  _parametersArray[2] = 1.0f;        // Sun intensity factor for sky
  _parametersArray[3] = _cycleTime;  // Current time for procedural sky effects

  // --- Shadow Map Pass ---
  bgfx::setViewFrameBuffer(SHADOW_MAP_VIEW_ID,
                           shadowMapFB);  // Set render target to shadow map
  bgfx::setViewRect(SHADOW_MAP_VIEW_ID, 0, 0, KNOWN_SHADOW_MAP_SIZE,
                    KNOWN_SHADOW_MAP_SIZE);  // Set viewport for shadow map
  bgfx::setViewClear(SHADOW_MAP_VIEW_ID, BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f,
                     0);  // Clear depth buffer

  // Setup light view and projection matrices for shadow mapping
  float lightViewMatrix[16], lightProjMatrix[16];
  float terrainWorldSize =
      (TerrainSize - 1) *
      TerrainScale;  // Calculate actual world size of terrain
  bx::Vec3 actualLightTarget = {
      terrainWorldSize * 0.5f, TERRAIN_MAX_ACTUAL_HEIGHT * 0.25f,
      terrainWorldSize * 0.5f};  // Light targets center of terrain
  bx::Vec3 lightPos =
      bx::mad(sunDirectionVec, -terrainWorldSize * 1.5f,
              actualLightTarget);  // Position light based on sun direction and
                                   // terrain size
  bx::mtxLookAt(lightViewMatrix, lightPos, actualLightTarget,
                bx::Vec3(0.0f, 1.0f, 0.0f));  // Light's view matrix

  float orthoHalfSize =
      terrainWorldSize *
      0.6f;  // Orthographic projection size covering the terrain
  bx::mtxOrtho(
      lightProjMatrix, -orthoHalfSize, orthoHalfSize, -orthoHalfSize,
      orthoHalfSize, 0.1f,
      terrainWorldSize * 3.0f,  // Near and far planes for ortho projection
      0.0f,
      bgfx::getCaps()
          ->homogeneousDepth);  // Use BGFX caps for homogeneous depth
  bgfx::setViewTransform(SHADOW_MAP_VIEW_ID, lightViewMatrix,
                         lightProjMatrix);  // Set transform for shadow pass

  // Set uniforms and state for shadow pass
  float terrainParamsArr[4] = {
      TERRAIN_MAX_ACTUAL_HEIGHT,  // x: height scale
      0.0f,                       // y: unused
      TerrainScale,               // z: world step X
      TerrainScale                // w: world step Z
  };

  bgfx::setUniform(_terrainParamsUniform, terrainParamsArr);
  bgfx::setTexture(3, _heightUniform,
                   _heightTexture);  // Bind heightmap texture for displacement
  bgfx::setTransform(this->world_matrix);  // Set terrain world matrix
  bgfx::setVertexBuffer(
      0, _shadowVbh);  // Use shadow-specific VBO (ShadowVertex layout)
  bgfx::setIndexBuffer(_terrainIbh);  // Use common terrain IBO
  bgfx::setState(BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS |
                 BGFX_STATE_CULL_CW |
                 BGFX_STATE_MSAA);  // Standard shadow pass state
  bgfx::submit(
      SHADOW_MAP_VIEW_ID,
      _terrainShadowProgram);  // Submit terrain draw call for shadow map

  // --- Main Scene Pass ---
  bgfx::setViewRect(ViewID::SCENE, 0, 0, canvasViewportW,
                    canvasViewportH);  // Set viewport for main scene
  bgfx::setViewClear(ViewID::SCENE, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     0x303030ff, 1.0f, 0);  // Clear color and depth

  // Calculate inverse view and projection matrices for skybox
  float invViewMatrix[16], invProjMatrix[16];
  bx::mtxInverse(invViewMatrix, viewMatrix);
  bx::mtxInverse(invProjMatrix, projMatrix);

  // Render Skybox
  bgfx::setViewTransform(
      ViewID::SCENE, viewMatrix,
      projMatrix);  // Set main camera view/proj for this view ID
  float skyboxModelMatrix[16];
  bx::mtxIdentity(
      skyboxModelMatrix);  // Skybox is at origin, transformed in shader
  bgfx::setTransform(skyboxModelMatrix);
  bgfx::setVertexBuffer(0, _skyVbh);  // Skybox quad vertices
  bgfx::setIndexBuffer(_skyIbh);      // Skybox quad indices
  bgfx::setUniform(_viewInvUniform,
                   invViewMatrix);  // Inverse view for ray direction
  bgfx::setUniform(_projInvUniform,
                   invProjMatrix);  // Inverse projection for ray direction
  bgfx::setUniform(_sunDirUniform,
                   sunDirShader);  // Sun direction for sky color
  bgfx::setUniform(_sunLumUniform,
                   _sunColorArray);  // Sun luminance for sky color
  bgfx::setUniform(_paramsUniform,
                   _parametersArray);  // Sky scattering parameters
  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                 BGFX_STATE_DEPTH_TEST_LEQUAL);  // Skybox render state (LEQUAL
                                                 // for sky at infinity)
  bgfx::submit(ViewID::SCENE, _skyProgram);      // Submit skybox draw call

  // Render Terrain
  uint8_t terrainViewID =
      ViewID::SCENE_N(1);  // Use a subsequent view ID for terrain (can share
                           // SCENE if no specific sorting needed)
  bgfx::setViewRect(terrainViewID, 0, 0, canvasViewportW,
                    canvasViewportH);  // Set viewport (same as main scene)
  bgfx::setViewTransform(terrainViewID, viewMatrix,
                         projMatrix);  // Set main camera view/proj for terrain

  bgfx::setTransform(this->world_matrix);  // Set terrain world matrix

  // Set terrain-specific uniforms
  float camPos[4];
  camera.getPosition(camPos);  // Get camera world position
  camPos[3] = 1.0f;            // Shader expects vec4
  bgfx::setUniform(_cameraPosUniform, camPos);
  bgfx::setUniform(_sunDirUniform, sunDirShader);
  bgfx::setUniform(_sunLumUniform, _sunColorArray);
  bgfx::setUniform(_skyAmbientUniform,
                   _skyAmbientArray);  // Ambient light for terrain

  // Setup light matrix for shadow sampling (transforms world to shadow map UV
  // space)
  float biasMatrix[16] = {
      // Matrix to transform clip space [-1,1] to texture space [0,1]
      0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f,
  };
  float lightVPMatrix[16];
  bx::mtxMul(lightVPMatrix, lightProjMatrix,
             lightViewMatrix);  // Light's View-Projection matrix
  float finalLightMatrixForShader[16];
  bx::mtxMul(finalLightMatrixForShader, biasMatrix,
             lightVPMatrix);  // Transform to shadow map sample space
  bgfx::setUniform(_lightMatrixUniform, finalLightMatrixForShader);

  bgfx::setUniform(_terrainParamsUniform,
                   terrainParamsArr);  // Terrain height scale
  float hmTexelSize[4] = {1.0f / TerrainSize, 1.0f / TerrainSize, 0.0f,
                          0.0f};  // Texel size for heightmap sampling
  bgfx::setUniform(_heightmapTexelSizeUniform, hmTexelSize);

  // Bind terrain textures
  bgfx::setTexture(0, _s_diffuseUniform, terrainDiffuse);
  bgfx::setTexture(1, _s_ormUniform, terrainORM);
  bgfx::setTexture(2, _s_normalUniform, terrainNormal);
  bgfx::setTexture(3, _heightUniform,
                   _heightTexture);  // Heightmap for displacement
  bgfx::setTexture(4, _shadowSamplerUniform,
                   shadowMapTexture);  // Shadow map for shadowing

  // Set terrain geometry and render state
  bgfx::setVertexBuffer(0, _terrainVbh);  // Terrain vertices
  bgfx::setIndexBuffer(_terrainIbh);      // Terrain indices
  bgfx::setState(
      BGFX_STATE_DEFAULT | BGFX_STATE_CULL_CW |
      BGFX_STATE_MSAA);  // Default render state with backface culling and MSAA
  /*
  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z
    | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA);
    // Use this state if culling is not needed, but depth testing is
  */
  bgfx::submit(terrainViewID,
               _terrainProgramHeight);  // Submit terrain draw call
}

// Releases all BGFX resources
void GameTest::Shutdown() {
  Logger::getInstance().Log(LogLevel::Debug, "[GameTest] Shutdown() called.");

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

  // Destroy textures
  destroyHandle(_heightTexture);
  destroyHandle(terrainDiffuse);
  destroyHandle(terrainORM);
  destroyHandle(terrainNormal);
  destroyHandle(shadowMapTexture);

  // Destroy framebuffers
  destroyHandle(shadowMapFB);

  // Destroy vertex and index buffers
  destroyHandle(_terrainVbh);
  destroyHandle(_terrainIbh);
  destroyHandle(_skyVbh);
  destroyHandle(_skyIbh);
  destroyHandle(_shadowVbh);

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

  Logger::getInstance().Log(LogLevel::Debug, "[GameTest] Shutdown() completed");
}