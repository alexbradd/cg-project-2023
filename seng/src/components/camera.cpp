#include <seng/application.hpp>
#include <seng/components/camera.hpp>
#include <seng/components/transform.hpp>
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
using namespace std;

using namespace std::placeholders;

std::vector<Camera*> Camera::cameras;

Camera::Camera(Entity& e, float near, float far, float fov, bool main) : BaseComponent(e)
{
  m_near = near;
  m_far = far;
  m_fov = fov;

  auto& window = entity->application().window();

  auto windowSize = window->framebufferSize();
  m_aspectRatio = windowSize.first / static_cast<float>(windowSize.second);

  window->onResize(std::bind(&Camera::resize, this, _2, _3));

  cameras.push_back(this);

  if (main) entity->scene().mainCamera(this);
}

Camera::~Camera()
{
  auto end = std::remove(cameras.begin(), cameras.end(), this);
  cameras.erase(end, cameras.end());
}

glm::mat4 Camera::projectionMatrix() const
{
  if (m_projectionDirty) {
    m_projection = glm::perspective(m_fov, m_aspectRatio, m_near, m_far);
    m_projection[1][1] *= -1;
    m_projectionDirty = false;
  }
  return m_projection;
}

glm::mat4 Camera::viewMatrix() const
{
  return glm::inverse(entity->transform()->toMat4());
}

void Camera::resize(int width, int height)
{
  float newAr = width / static_cast<float>(height);
  if (newAr != m_aspectRatio) {
    m_projectionDirty = true;
    m_aspectRatio = newAr;
  }
}

void Camera::nearPlane(float near)
{
  m_projectionDirty = true;
  m_near = near;
}

void Camera::farPlane(float far)
{
  m_projectionDirty = true;
  m_far = far;
}

void Camera::fov(float fov)
{
  m_projectionDirty = true;
  m_fov = fov;
}

DEFINE_CREATE_FROM_CONFIG(Camera, entity, node)
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
