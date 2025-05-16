#ifndef ORBIT_CAMERA_H
#define ORBIT_CAMERA_H

#include <bgfx/bgfx.h>
#include <bx/math.h>

class OrbitCamera {
public:
  OrbitCamera()
      : m_distance(100.0f),
        m_pitch(0.5f),
        m_yaw(0.0f),
        m_roll(0.0f),
        m_target{0.0f, 0.0f, 0.0f} {}

  void orbit(float dx, float dy) { // ORBIT
    constexpr float kMouseSpeed = 0.005f;
    m_yaw += dx * kMouseSpeed;
    m_pitch += dy * kMouseSpeed;

    const float limit = bx::kPiHalf - 0.01f;
    m_pitch = bx::clamp(m_pitch, -limit, limit);
  }

  void zoom(float dz) { // ZOOM
    constexpr float kZoomSpeed = 0.1f;
    m_distance = bx::clamp(m_distance + dz * kZoomSpeed, +1.0f, 500.0f);
  }

  void pan(float dx, float dy) { // PAN
    float offset[3] = {};
    calcPanOffset(dx, dy, offset);
    for (int i = 0; i < 3; ++i) {
      m_target[i] += offset[i];
    }
  }

  void getViewMatrix(float* out) const {
    float eye[3];
    getPosition(eye);

    // 1: Create basic view matrix from eye -> target
    float view[16];
    bx::mtxLookAt(view, bx::Vec3{eye[0], eye[1], eye[2]},
                  bx::Vec3{m_target[0], m_target[1], m_target[2]},
                  bx::Vec3{0.0f, 1.0f, 0.0f});

    if (m_roll != 0.0f) {
      // 2: Extract the rotation part of the view matrix
      // Apply roll in camera local space (around forward Z)

      float rollMtx[16];
      bx::mtxIdentity(rollMtx);
      bx::mtxRotateZ(rollMtx, m_roll);  // Roll around camera-local Z

      // 3: Combine roll with view matrix
      // This applies the roll as a twist around the forward axis
      bx::mtxMul(out, rollMtx, view);
    } else {
      // No roll; copy base view matrix
      bx::memCopy(out, view, sizeof(view));
    }
  }



  void getProjectionMatrix(float* out, float aspect, float fov = 60.0f) const {
    bx::mtxProj(out, fov, aspect, 0.1f, 1000.0f,
                bgfx::getCaps()->homogeneousDepth);
  }

  void getPosition(float* out) const {
    float cp = bx::cos(m_pitch);
    float sp = bx::sin(m_pitch);
    float cy = bx::cos(m_yaw);
    float sy = bx::sin(m_yaw);

    out[0] = m_target[0] + m_distance * cp * sy;
    out[1] = m_target[1] + m_distance * sp;
    out[2] = m_target[2] + m_distance * cp * cy;
  }

  // Getters/setters for UI
  float getDistance() const { return m_distance; }
  float getPitch() const { return m_pitch; }
  float getYaw() const { return m_yaw; }
  float getRoll() const { return m_roll; }
  const float* getTarget() const { return m_target; }

  void setDistance(float d) { m_distance = d; }
  void setPitch(float p) { m_pitch = p; }
  void setYaw(float y) { m_yaw = y; }
  void setRoll(float r) { m_roll = r; }
  void setTarget(const float* t) {
    m_target[0] = t[0];
    m_target[1] = t[1];
    m_target[2] = t[2];
  }

private:
  void calcPanOffset(float dx, float dy, float* out) const {
    float eye[3];
    getPosition(eye);

    float forwardVec[3] = {m_target[0] - eye[0], m_target[1] - eye[1],
                           m_target[2] - eye[2]};

    bx::Vec3 forward = bx::normalize(bx::load<bx::Vec3>(forwardVec));

    static const float worldUp[3] = {0.0f, 1.0f, 0.0f};
    bx::Vec3 up = bx::load<bx::Vec3>(worldUp);

    bx::Vec3 right = bx::normalize(bx::cross(up, forward));
    bx::Vec3 newUp = bx::cross(forward, right);

    float panScale = m_distance * 0.001f;
    bx::Vec3 pan = bx::mad(right, dx * panScale, bx::mul(newUp, dy * panScale));

    bx::store(out, pan);
  }

  // Declare the camera parameters
  float m_distance;
  float m_pitch;
  float m_yaw;
  float m_roll;
  float m_target[3];
};

#endif  // ORBIT_CAMERA_H
