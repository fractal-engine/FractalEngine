#include "imgizmo.h"

#include <bgfx/bgfx.h>

#include "engine/context/engine_context.h"
#include "engine/core/logger.h"
#include "engine/math/transformation.h"

// Global resource
IMGizmo::StaticData IMGizmo::static_data_;

// TODO: add icon_scale
IMGizmo::IMGizmo()
    : color(glm::vec3(1.0f)),
      opacity(1.0f),
      foreground(false),
      shape_render_stack_(),
      icon_render_stack_() {}

void IMGizmo::Create() {
  if (static_data_.loaded) {

    ResourceManager& resource = EngineContext::resourceManager();

    // TODO: Load shape meshes for component gizmos, include icons
    // handle shaders
    // auto [boxId, box] =
    // EngineContext::resourceManager().Create<Model>("gizmo-box");
    // box->Load("resources/primitives/box.fbx");
    // static_data_.box_mesh = box->QueryMesh(0);

    // Initialize line vertex layout
    static_data_.line_layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    // Load gizmo shaders
    // TODO: use 'shader_pool' instead
    static_data_.line_shader = EngineContext::Shader().LoadProgram(
        "gizmo", "vs_grid_plane.bin", "fs_grid_plane.bin");

    if (!bgfx::isValid(static_data_.line_shader)) {
      Logger::getInstance().Log(LogLevel::Warning,
                                "IMGizmo: Failed to load line shader, "
                                "using fallback rendering");
    }

    // Create plane mesh
    static_data_.plane_mesh = CreatePlaneMesh();
  }
  return;

  static_data_.loaded = true;
  Logger::getInstance().Log(LogLevel::Debug, "IMGizmo: Initialized");
}

void IMGizmo::NewFrame(bgfx::ViewId view_id) {
  primitive_render_stack_.clear();
  shape_render_stack_.clear();
  // icon_render_stack_.clear();
  current_view_id_ = view_id;
}

void IMGizmo::RenderAll(const glm::mat4& view_projection) {
  if (primitive_render_stack_.empty())
    return;

  RenderPrimitives(view_projection);
  RenderShapes(view_projection);
  // RenderIcons(view_projection);
}

// RENDER
void IMGizmo::RenderPrimitives(const glm::mat4& view_projection) {
  if (!bgfx::isValid(static_data_.line_shader))
    return;

  // TODO: Implement line rendering
  // requires creating dynamic vertex buffer
  // and using bgfx::TransientVertexBuffer for line segments

  // idea for future implementation:
  // 1. Create transient vertex buffer for all lines
  // 2. Pack line vertices with position + color
  // 3. Use Transformation::Swap for coordinate conversion
  // 4. Submit with line shader and BGFX_STATE_PT_LINES

  // ! STUB
  const uint32_t num_lines =
      static_cast<uint32_t>(primitive_render_stack_.size());
  if (num_lines == 0)
    return;

  // Each line = 2 vertices
  const uint32_t num_vertices = num_lines * 2;

  // Allocate transient vertex buffer
  if (!bgfx::getAvailTransientVertexBuffer(num_vertices,
                                           static_data_.line_layout))
    return;

  bgfx::TransientVertexBuffer tvb;
  bgfx::allocTransientVertexBuffer(&tvb, num_vertices,
                                   static_data_.line_layout);

  // Pack vertices (Position + Color)
  struct LineVertex {
    float x, y, z;
    uint32_t abgr;
  };

  LineVertex* vertex = (LineVertex*)tvb.data;

  for (const auto& primitive : primitive_render_stack_) {
    // Swap coordinates
    glm::vec3 start = primitive.start;
    glm::vec3 end = primitive.end;

    // Pack color (ABGR format)
    uint8_t r = static_cast<uint8_t>(primitive.state.color.r * 255.0f);
    uint8_t g = static_cast<uint8_t>(primitive.state.color.g * 255.0f);
    uint8_t b = static_cast<uint8_t>(primitive.state.color.b * 255.0f);
    uint8_t a = static_cast<uint8_t>(primitive.state.opacity * 255.0f);
    uint32_t abgr = (a << 24) | (b << 16) | (g << 8) | r;

    // Start vertex
    vertex->x = start.x;
    vertex->y = start.y;
    vertex->z = start.z;
    vertex->abgr = abgr;
    vertex++;

    // End vertex
    vertex->x = end.x;
    vertex->y = end.y;
    vertex->z = end.z;
    vertex->abgr = abgr;
    vertex++;
  }

  // Set vertex buffer and render state
  bgfx::setVertexBuffer(0, &tvb);
  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                 BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_PT_LINES);

  bgfx::submit(current_view_id_, static_data_.line_shader);
}

void IMGizmo::RenderShapes(const glm::mat4& view_projection) {
  if (!bgfx::isValid(static_data_.line_shader))
    return;

  for (int32_t i = 0; i < shape_render_stack_.size(); i++) {
    ShapeRenderTarget gizmo = shape_render_stack_[i];
  }
}

void IMGizmo::RenderIcons(const glm::mat4& view_projection) {
  if (!bgfx::isValid(static_data_.line_shader))
    return;
}

// PRIMITIVES
void IMGizmo::Line(const glm::vec3& start, const glm::vec3& end) {
  PrimitiveRenderTarget primitive(start, end, GetCurrentState());
  primitive_render_stack_.push_back(primitive);
}

// SHAPES
void IMGizmo::Plane(const glm::vec3& position, const glm::vec3& scale,
                    const glm::quat& rotation) {
  /* ShapeRenderTarget gizmo(Shape::PLANE, position, rotation, scale, false,
                          GetCurrentState());*/

  // shape_render_stack_.push_back(gizmo);
}

// ICONS

IMGizmo::RenderState IMGizmo::GetCurrentState() {
  RenderState state;
  state.color = color;
  state.opacity = opacity;
  state.foreground = foreground;
  return state;
}

const Mesh* IMGizmo::QueryMesh(Shape shape) {
  switch (shape) {
    case Shape::PLANE:
      return static_data_.plane_mesh;
    default:
      return nullptr;
  }
}

glm::mat4 IMGizmo::GetModelMatrix(glm::vec3 position, glm::quat rotation,
                                  glm::vec3 scale) {
  glm::mat4 model(1.0f);

  // Position
  glm::vec3 worldPos = position;
  model = glm::translate(model, worldPos);

  // Rotation
  glm::quat worldRot = rotation;
  model = model * glm::mat4_cast(glm::normalize(worldRot));

  // Scale
  model = glm::scale(model, scale);

  return model;
}

const Mesh* IMGizmo::CreatePlaneMesh() {
  // Create plane vertices (XZ plane at Y=0)
  Resources3D::MeshData plane_data;

  // Positions (4 corners)
  plane_data.positions_ = {
      -1.0f, 0.0f, -1.0f,  // 0
      1.0f,  0.0f, -1.0f,  // 1
      1.0f,  0.0f, 1.0f,   // 2
      -1.0f, 0.0f, 1.0f    // 3
  };

  // Normals (all pointing up)
  plane_data.normals_ = {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                         0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f};

  // Indices (two triangles)
  plane_data.indices_ = {0, 1, 2, 0, 2, 3};

  return new Mesh(plane_data);
}
