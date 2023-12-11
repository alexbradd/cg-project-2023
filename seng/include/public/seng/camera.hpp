#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include <glm/mat4x4.hpp>
#include <seng/transform.hpp>

namespace seng {

/**
 * Representation of the device through which we view the scene.
 *
 * Bascially, it a wrapper around a Transform and the usual camera parameters
 * (near and far planes, aspect ratio and fov).
 *
 * One particular thing to note: the camera is orineted along the -z axis,
 * meaning that translating along `transform().forward()` moves the camera backwards.
 */
class Camera {
 public:
  Camera(float aspectRatio,
         float near = 0.1f,
         float far = 1000.0f,
         float fov = glm::radians(45.0f));
  Camera(const Camera&) = delete;
  Camera(Camera&&) = default;

  Camera& operator=(const Camera&) = delete;
  Camera& operator=(Camera&&) = default;

  // Getters
  float aspectRatio() const { return _aspectRatio; }
  float nearPlane() const { return _near; }
  float farPlane() const { return _far; }
  float fov() const { return _fov; }
  const Transform& transform() const { return _transform; }

  // Setters
  void aspectRatio(float ar);
  void nearPlane(float near);
  void farPlane(float far);
  void fov(float fov);
  Transform& transform();

  /**
   * Returns the projection matrix for this camera
   */
  glm::mat4 projectionMatrix() const;

  /**
   * Returns the view matrix for this camera
   */
  glm::mat4 viewMatrix() const;

 private:
  float _aspectRatio;
  float _near;
  float _far;
  float _fov;
  Transform _transform;

  // For caching
  mutable bool _projectionDirty = true, _viewDirty = true;
  mutable glm::mat4 _projection;
  mutable glm::mat4 _view;
};

};  // namespace seng
