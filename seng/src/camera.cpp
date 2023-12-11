#include <seng/camera.hpp>

// clang-format off
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// clang-format on

using namespace seng;
using namespace glm;

Camera::Camera(float aspectRatio, float near, float far, float fov) :
    _aspectRatio(aspectRatio), _near(near), _far(far), _fov(fov)
{
}

mat4 Camera::projectionMatrix() const
{
  if (_projectionDirty) {
    _projection = perspective(_fov, _aspectRatio, _near, _far);
    _projection[1][1] *= -1;
    _projectionDirty = false;
  }
  return _projection;
}

mat4 Camera::viewMatrix() const
{
  if (_viewDirty) {
    _view = glm::inverse(_transform.toMat4());
    _viewDirty = false;
  }
  return _view;
}

Transform& Camera::transform()
{
  _viewDirty = true;
  return _transform;
}

void Camera::aspectRatio(float ar)
{
  _projectionDirty = true;
  _aspectRatio = ar;
}

void Camera::nearPlane(float near)
{
  _projectionDirty = true;
  _near = near;
}

void Camera::farPlane(float far)
{
  _projectionDirty = true;
  _far = far;
}

void Camera::fov(float fov)
{
  _projectionDirty = true;
  _fov = fov;
}
