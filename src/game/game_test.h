#ifndef GAME_TEST_H
#define GAME_TEST_H

#include <bgfx/bgfx.h>
#include <vector>
#include "editor/components/orbit_camera.h"
#include "editor/systems/camera_system.h"
#include "game/game_base.h"

// ──────────────────────────────────────────────────────
//  Vertex layout used by terrain + sun billboard
// ──────────────────────────────────────────────────────
struct PosTexCoord0Vertex {
  float x, y, z;
  float u, v;

  static bgfx::VertexLayout layout;
  static void init();
};

// ──────────────────────────────────────────────────────
//  GameTest
// ──────────────────────────────────────────────────────
class GameTest : public GameBase {
public:
  GameTest();
  ~GameTest() override;

  void Init() override;
  void Update() override;
  void Render() override;
  void Shutdown() override;

  // simple camera function call
  OrbitCamera camera;
  CameraSystem cameraSystem;

  int canvasViewportW = 800;
  int canvasViewportH = 600;

private:
  // ───── Terrain
  bgfx::ProgramHandle _terrainProgramHeight = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _heightUniform = BGFX_INVALID_HANDLE;

  bgfx::TextureHandle _heightTexture = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle terrainDiffuse = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle terrainORM = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle terrainNormal = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _cameraPosUniform = BGFX_INVALID_HANDLE;


  bgfx::UniformHandle _lightDirUniform = BGFX_INVALID_HANDLE;

  bgfx::VertexBufferHandle _terrainVbh = BGFX_INVALID_HANDLE;
  bgfx::IndexBufferHandle _terrainIbh = BGFX_INVALID_HANDLE;

  std::vector<PosTexCoord0Vertex> terrainVertices;
  std::vector<uint16_t> terrainIndices;

  // ───── Sky-box & Sun
  bgfx::ProgramHandle _skyProgram = BGFX_INVALID_HANDLE;
  bgfx::ProgramHandle _sunProgram = BGFX_INVALID_HANDLE;

  bgfx::VertexBufferHandle _skyVbh = BGFX_INVALID_HANDLE;
  bgfx::IndexBufferHandle _skyIbh = BGFX_INVALID_HANDLE;

  bgfx::UniformHandle _timeUniform = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _sunDirUniform = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _sunLumUniform = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _paramsUniform = BGFX_INVALID_HANDLE;

  // New skybox uniforms
  bgfx::UniformHandle _viewInvUniform = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _projInvUniform = BGFX_INVALID_HANDLE;

  // Texture Uniforms

  bgfx::UniformHandle _s_diffuseUniform = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _s_ormUniform = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _s_normalUniform = BGFX_INVALID_HANDLE;


  float _cycleTime = 0.0f;  // day-night timerm keep it at 0

  // colour / param arrays passed to both sky & sun shaders
  float _sunColorArray[4] = {5.0f, 5.0f, 5.0f, 0.0f};
  float _parametersArray[4] = {1.0f, 1.0f, 1.0f, 0.0f};

  // small helpers that build vertex / index buffers
  void createSkyboxBuffers();

  // world transform for the terrain
  float world_matrix[16];
};

#endif  // GAME_TEST_H
