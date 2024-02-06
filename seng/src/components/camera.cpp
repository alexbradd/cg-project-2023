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
#include <glm/trigonometric.hpp>
#include <glm/gtc/reciprocal.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
// clang-format on
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <memory>
#include <vector>

using namespace seng;
using namespace std;

using namespace std::placeholders;

std::vector<Camera*> Camera::cameras;

Camera::Camera(
    Entity& e, bool main, float near, float far, float fov, bool ortho, float halfWidth) :
    BaseComponent(e)
{
  m_ortho = ortho;
  m_half = halfWidth;
  m_near = near;
  m_far = far;
  m_fov = fov;

  auto& window = entity->application().window();
  m_resizeToken = window->onResize().insert(std::bind(&Camera::resize, this, _2, _3));

  auto windowSize = window->framebufferSize();
  m_aspectRatio = windowSize.first / static_cast<float>(windowSize.second);

  cameras.push_back(this);

  if (main) entity->scene().mainCamera(this);
}

Camera::~Camera()
{
  entity->application().window()->onResize().remove(m_resizeToken);
  auto end = std::remove(cameras.begin(), cameras.end(), this);
  cameras.erase(end, cameras.end());
}

glm::mat4 Camera::projectionMatrix() const
{
  if (m_projectionDirty) {
    if (m_ortho) {
      float top = m_half / m_aspectRatio;
      float bottom = -top;
      float left = -m_half;
      float right = -left;
      m_projection = glm::scale(glm::orthoLH_ZO(left, right, bottom, top, m_near, m_far),
                                glm::vec3(1.0f, -1.0f, 1.0f));
    } else {
      m_projection = glm::perspectiveLH_ZO(m_fov, m_aspectRatio, m_near, m_far);
      m_projection[1][1] *= -1;
    }
    m_projectionDirty = false;
  }
  return m_projection;
}

glm::mat4 Camera::viewMatrix() const
{
  if (entity->transform()->changed()) {
    m_view = glm::inverse(entity->transform()->worldMartix());
    entity->transform()->clearChanged();
  }
  return m_view;
}

void Camera::resize(int width, int height)
{
  float newAr = width / static_cast<float>(height);
  if (newAr != m_aspectRatio) {
    m_projectionDirty = true;
    m_aspectRatio = newAr;
  }
}

void Camera::orthographic(bool orthographic)
{
  m_projectionDirty = true;
  m_ortho = orthographic;
}

void Camera::halfWidth(float halfWidth)
{
  m_projectionDirty = true;
  m_half = halfWidth;
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
  bool ortho = DEFAULT_ORTHO;
  float halfWidth = DEFAULT_HALFWIDTH;

  if (node["main"] && node["main"].IsScalar()) main = node["main"].as<bool>();
  if (node["orthographic"] && node["orthographic"].IsScalar())
    ortho = node["orthographic"].as<bool>();
  if (node["half_width"] && node["half_width"].IsScalar())
    halfWidth = node["half_width"].as<float>(DEFAULT_HALFWIDTH);
  if (node["near"] && node["near"].IsScalar())
    near = node["near"].as<float>(DEFAULT_NEAR);
  if (node["far"] && node["far"].IsScalar()) far = node["far"].as<float>(DEFAULT_FAR);
  if (node["fov_deg"] && node["fov_deg"].IsScalar()) {
    float deg = node["fov_deg"].as<float>(45.0f);
    fov = glm::radians(deg);
  }
  if (node["fov_radians"] && node["fov_radians"].IsScalar())
    fov = node["fov_radians"].as<float>(DEFAULT_FOV);

  return std::make_unique<Camera>(entity, main, near, far, fov, ortho, halfWidth);
}
