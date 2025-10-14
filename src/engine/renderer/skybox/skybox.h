#ifndef SKYBOX_H
#define SKYBOX_H

#include <bgfx/bgfx.h>
#include <bx/math.h>

#include "engine/memory/resource_manager.h"
#include "engine/renderer/shaders/shader_manager.h"

// Parameters you may want to read from other passes (terrain/water).
struct SkyboxParams {
  float _parametersArray[4];  // { Sun Angular Radius, Procedural Bloom,
                              // Exposure, Time }
  float _sunColorArray[4];    // sun RGB + intensity (w)
  float _skyAmbientArray[4];  // ambient RGB (+ unused w)
  float sunDirShader[4];      // sun direction (xyz, 0)
  float totalBetaR[4];        // Rayleigh totals (xyz, 0)
  float totalBetaM[4];        // Mie totals (xyz, 0)
  float scatterParams[4];     // { g, 0,0,0 }
  float _cycleTime = 0.0f;
};

class Skybox {
public:
  Skybox() = default;

  // Create shaders, uniforms, and fullscreen quad.
  // Expects Runtime::Shader()->LoadProgram(...) to be available globally (as in
  // your codebase).
  void Create(ShaderManager* ShaderManager);

  // Destroy all BGFX handles.
  void Destroy();

  // Advance time (seconds or arbitrary units) and recompute sun/ambient.
  void Update(float dt);

  // Manually tweak skybox UI-ish params (not time).
  // (time is taken from Update / _cycleTime).
  void SetParams(float sunAngularRadius, float proceduralBloom, float exposure);

  // Recompute sun direction, luminance, ambient, and scattering from current
  // _cycleTime.
  void ComputeSun();

  // Submit a draw for the skybox to the given view.
  // If useInverseViewProj==true: we compute inverse(view/proj) and set
  // u_viewInv/u_projInv accordingly (this matches your main skybox path). If
  // false: we assume the caller passed a "rotation-only" view (e.g.,
  // reflection) and set uniforms directly (this matches your reflection path).
  void Submit(bgfx::ViewId viewId, const float* viewMatrix,
              const float* projMatrix, bool useInverseViewProj);

  // Give access to the computed parameters (for terrain/water).
  const SkyboxParams& GetParams() const { return params_; }

  // Explicit helper if you need to rebuild just the quad.
  void CreateFullscreenQuad();

private:
  // Geometry for fullscreen quad
  struct ScreenPosVertex {
    float x, y;
    static bgfx::VertexLayout layout;
    static void initLayout() {
      if (!layout.m_stride) {
        layout.begin()
            .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
            .end();
      }
    }
  };

  // Handles (same names as you used)
  bgfx::ProgramHandle _skyProgram = BGFX_INVALID_HANDLE;
  bgfx::VertexBufferHandle _skyVbh = BGFX_INVALID_HANDLE;
  bgfx::IndexBufferHandle _skyIbh = BGFX_INVALID_HANDLE;

  // Uniforms (same names)
  bgfx::UniformHandle _sunDirUniform = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _sunLumUniform = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _paramsUniform = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _viewInvUniform = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _projInvUniform = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _scatterParamsUniform = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _betaRUniform = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle _betaMUniform = BGFX_INVALID_HANDLE;

  // Stored values/params (same arrays/vars)
  SkyboxParams params_{};

  // Constants used by your original code (no logic changes)
  const float H_R = 8000.0f;
  const float H_M = 1200.0f;
  const float k_betaR_per_meter[3] = {5.8e-6f, 13.5e-6f, 33.1e-6f};
  const float MieColoration[3] = {1.0f, 0.95f, 0.85f};
  const float MieBaseDensity = 20.0e-6f;
};

#endif  // SKYBOX_H