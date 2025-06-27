#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <bgfx/bgfx.h>  // For bgfx handles
#include <glm/glm.hpp>  // For transform matrix
#include <string>

/*********************************************************************************
 * @class GameObject
 * @brief Basic game object with an ID and a name
 * @see GameObject.cpp
 *
 * This class serves as a minimal base for all in-game objects. It holds:
 *   - A unique ID (integer)
 *   - A name (string)
 *
 * Future Ideas:
 *   - Position/Rotation: Provide methods to track object transform.
 *   - Collision support: Enable collision detection and response.
 *   - Inheritance/Components: Extend via derived classes.
 *
 * Development:
 *   - Tied to GameObject.cpp, which contains the class implementation.
 *   - New methods or attributes should be declared here.
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

  /*********************************************************************************
   * @brief Sets the world transform matrix of the object
   * @param transform A 4x4 transformation matrix
   **********************************************************************************/
  void SetTransform(const glm::mat4& transform);

  /*********************************************************************************
   * @brief Gets the current world transform matrix
   * @return Reference to the internal transformation matrix
   **********************************************************************************/
  const glm::mat4& GetTransform() const;

  /*********************************************************************************
   * @brief Assigns BGFX vertex and index buffers to this object
   * @param vbo Vertex buffer handle
   * @param ibo Index buffer handle
   **********************************************************************************/
  void SetBuffers(bgfx::VertexBufferHandle vbo, bgfx::IndexBufferHandle ibo);

  /*********************************************************************************
   * @brief Render the object using assigned BGFX buffers and shader
   **********************************************************************************/
  void Render();

  /*********************************************************************************
   * @brief Assigns a texture handle to the GameObject
   * @param texture A valid bgfx::TextureHandle
   **********************************************************************************/
  void SetTexture(bgfx::TextureHandle texture);
  void SetORMTexture(bgfx::TextureHandle texture);
  void SetNormalTexture(bgfx::TextureHandle texture);

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

  // Actually Idk why we're using this DOxygen briefs, but it seems to be in the whole case and Intellisense just copies it

private:
  int id_;
  std::string name_;

  // -- New attributes for transform and rendering --
  glm::mat4 transform_{1.0f};  // Identity matrix
  bgfx::VertexBufferHandle vbo_ = BGFX_INVALID_HANDLE;
  bgfx::IndexBufferHandle ibo_ = BGFX_INVALID_HANDLE;

  bgfx::TextureHandle texture_ = BGFX_INVALID_HANDLE;     // Diffuse/BaseColor
  bgfx::TextureHandle ormTexture_ = BGFX_INVALID_HANDLE;  // ORM texture
  bgfx::TextureHandle normalTexture_ = BGFX_INVALID_HANDLE;  // Normal map

  static bgfx::UniformHandle s_diffuseSampler_;
  static bgfx::UniformHandle s_ormSampler_;
  static bgfx::UniformHandle s_normalSampler_;

};

#endif  // GAMEOBJECT_H
