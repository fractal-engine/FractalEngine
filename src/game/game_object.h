#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <bgfx/bgfx.h>  // For bgfx handles
#include <glm/glm.hpp>  // For transform matrix
#include <string>

/*********************************************************************************
 * @struct RenderContext
 * @brief  Holds shared scene information needed by an object for rendering.
 *
 * This struct bundles all the global, per-frame lighting and camera data that
 * a shader needs but that is not specific to the GameObject itself. An instance
 * of this is created in the main render loop and passed to each object.
 **********************************************************************************/
struct RenderContext {
  // Uniform Handles (to identify the uniform in the shader)
  bgfx::UniformHandle u_cameraPos;
  bgfx::UniformHandle u_sunDirection;
  bgfx::UniformHandle u_sunLuminance;
  bgfx::UniformHandle u_skyAmbient;
  bgfx::UniformHandle u_lightMatrix;
  bgfx::UniformHandle u_shadowSampler;

  // Pointers to the actual uniform data arrays
  const float* cameraPosValue;
  const float* sunDirValue;
  const float* sunLuminanceValue;
  const float* skyAmbientValue;
  const float* lightMatrixValue;

  // Handle to the shared shadow map texture
  bgfx::TextureHandle shadowMapTexture;
};

/*********************************************************************************
 * @class GameObject
 * @brief Basic game object with transform, mesh, and textures.
 **********************************************************************************/
class GameObject {
public:
  // Constructor
  GameObject(int id, const std::string& name);

  // Virtual destructor
  virtual ~GameObject();

  // Accessors
  int GetId() const;
  std::string GetName() const;
  const glm::mat4& GetTransform() const;
  bgfx::VertexBufferHandle GetVBO() const;
  bgfx::IndexBufferHandle GetIBO() const;

  /*********************************************************************************
   * @brief Sets the world transform matrix of the object
   * @param transform A 4x4 transformation matrix
   **********************************************************************************/
  void SetTransform(const glm::mat4& transform);

  /*********************************************************************************
   * @brief Assigns BGFX vertex and index buffers to this object
   * @param vbo Vertex buffer handle
   * @param ibo Index buffer handle
   **********************************************************************************/
  void SetBuffers(bgfx::VertexBufferHandle vbo, bgfx::IndexBufferHandle ibo);

  /*********************************************************************************
   * @brief Assigns a texture handle to the GameObject
   * @param texture A valid bgfx::TextureHandle
   **********************************************************************************/
  void SetTexture(bgfx::TextureHandle texture);
  void SetORMTexture(bgfx::TextureHandle texture);
  void SetNormalTexture(bgfx::TextureHandle texture);

  /*********************************************************************************
   * @brief Renders the object using its assigned buffers and textures.
   * @param context A struct containing shared scene lighting and camera info.
   *
   * This is the key change. The function now requires the RenderContext to
   * properly bind all uniforms and textures needed by the PBR shader.
   **********************************************************************************/
  void Render(const RenderContext& context);

  // --- Static Sampler Management ---

  /*********************************************************************************
   * @brief Setter for the static texture uniform handle
   * @param handle A bgfx::UniformHandle representing the diffuse sampler
   **********************************************************************************/
  static void SetDiffuseSampler(bgfx::UniformHandle handle);
  static void SetORMSampler(bgfx::UniformHandle handle);
  static void SetNormalSampler(bgfx::UniformHandle handle);

  /*********************************************************************************
   * @brief Accessor for the static texture uniform handle
   * @return bgfx::UniformHandle for diffuse sampler
   **********************************************************************************/
  static bgfx::UniformHandle GetDiffuseSampler();
  static bgfx::UniformHandle GetORMSampler();
  static bgfx::UniformHandle GetNormalSampler();

private:
  int id_;
  std::string name_;

  // -- Object-specific attributes --
  glm::mat4 transform_{1.0f};  // Identity matrix
  bgfx::VertexBufferHandle vbo_ = BGFX_INVALID_HANDLE;
  bgfx::IndexBufferHandle ibo_ = BGFX_INVALID_HANDLE;

  bgfx::TextureHandle texture_ = BGFX_INVALID_HANDLE;     // Diffuse/BaseColor
  bgfx::TextureHandle ormTexture_ = BGFX_INVALID_HANDLE;  // ORM texture
  bgfx::TextureHandle normalTexture_ = BGFX_INVALID_HANDLE;  // Normal map

  // -- Static handles shared by all GameObjects --
  static bgfx::UniformHandle s_diffuseSampler_;
  static bgfx::UniformHandle s_ormSampler_;
  static bgfx::UniformHandle s_normalSampler_;
};

#endif  // GAMEOBJECT_H