#include "orbit_camera.h"
#include <bx/math.h>

void OrbitCamera::GetViewMatrix(float* outView) const {
  float toEye[3];
  float cp = bx::cos(bx::toRad(m_pitch));
  float sp = bx::sin(bx::toRad(m_pitch));
  float cy = bx::cos(bx::toRad(m_yaw));
  float sy = bx::sin(bx::toRad(m_yaw));

  toEye[0] = m_distance * cp * sy;
  toEye[1] = m_distance * sp;
  toEye[2] = m_distance * cp * cy;

  float eye[3] = {
      m_target[0] + toEye[0],
      m_target[1] + toEye[1],
      m_target[2] + toEye[2],
  };

  bx::mtxLookAt(outView, bx::Vec3{eye[0], eye[1], eye[2]},
                bx::Vec3{m_target[0], m_target[1], m_target[2]},
                bx::Vec3{0.0f, 1.0f, 0.0f});
}

void OrbitCamera::Orbit(float dx, float dy) {
  m_yaw -= dx;
  m_pitch += dy;

  const float limit = 89.0f;
  if (m_pitch > limit)
    m_pitch = limit;
  if (m_pitch < -limit)
    m_pitch = -limit;
}

void OrbitCamera::Zoom(float amount) {
  m_distance -= amount;
  if (m_distance < 1.0f)
    m_distance = 1.0f;
}

void OrbitCamera::Pan(float dx, float dy) {
  float right[3], up[3];
  float view[16];

  GetViewMatrix(view);

  // Right = column 0, Up = column 1
  right[0] = view[0];
  right[1] = view[4];
  right[2] = view[8];
  up[0] = view[1];
  up[1] = view[5];
  up[2] = view[9];

  m_target[0] += right[0] * dx + up[0] * dy;
  m_target[1] += right[1] * dx + up[1] * dy;
  m_target[2] += right[2] * dx + up[2] * dy;
}
