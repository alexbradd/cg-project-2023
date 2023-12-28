#include <seng/application.hpp>
#include <seng/components/camera.hpp>
#include <seng/log.hpp>
#include <seng/rendering/glfw_window.hpp>
#include <seng/scene/entity.hpp>
#include <seng/scene/scene.hpp>

// clang-format off
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/trigonometric.hpp"
// clang-format on
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <memory>
#include <vector>

using namespace seng;
using namespace seng::scene;
using namespace seng::components;
using namespace std;

using namespace std::placeholders;

std::vector<Camera*> Camera::cameras;

Camera::Camera(Entity& entity, float near, float far, float fov, bool main) :
    BaseComponent(entity)
{
  _near = near;
  _far = far;
  _fov = fov;
  registerAsMain = main;
}

void Camera::initialize()
{
  auto& window = entity->getApplication().window();

  auto windowSize = window->framebufferSize();
  _aspectRatio = windowSize.first / static_cast<float>(windowSize.second);

  window->onResize(std::bind(&Camera::resize, this, _2, _3));

  cameras.push_back(this);

  if (registerAsMain) entity->getScene().setMainCamera(this);
}

Camera::~Camera()
{
  auto end = std::remove(cameras.begin(), cameras.end(), this);
  cameras.erase(end, cameras.end());
}

glm::mat4 Camera::projectionMatrix() const
{
  if (_projectionDirty) {
    _projection = glm::perspective(_fov, _aspectRatio, _near, _far);
    _projection[1][1] *= -1;
    _projectionDirty = false;
  }
  return _projection;
}

glm::mat4 Camera::viewMatrix() const
{
  return glm::inverse(entity->getTransform()->toMat4());
}

void Camera::resize(int width, int height)
{
  float newAr = width / static_cast<float>(height);
  if (newAr != _aspectRatio) {
    _projectionDirty = true;
    _aspectRatio = newAr;
  }
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

std::unique_ptr<BaseComponent> Camera::createFromConfig(Entity& entity,
                                                        const YAML::Node& node)
{
  float near = DEFAULT_NEAR;
  float far = DEFAULT_FAR;
  float fov = DEFAULT_FOV;
  bool main = DEFAULT_MAIN;

  if (node["main"] && node["main"].IsScalar()) main = node["main"].as<bool>();
  if (node["near"] && node["near"].IsScalar())
    near = node["near"].as<float>(DEFAULT_NEAR);
  if (node["far"] && node["far"].IsScalar()) far = node["far"].as<float>(DEFAULT_FAR);
  if (node["fov_deg"] && node["fov_deg"].IsScalar()) {
    float deg = node["fov_deg"].as<float>(45.0f);
    fov = glm::radians(deg);
  }
  if (node["fov_radians"] && node["fov_radians"].IsScalar())
    fov = node["fov_radians"].as<float>(DEFAULT_FOV);

  return std::make_unique<Camera>(entity, near, far, fov, main);
}
