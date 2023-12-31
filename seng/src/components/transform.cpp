#include <seng/components/transform.hpp>
#include <seng/yaml_utils.hpp>

#include <yaml-cpp/yaml.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

using namespace seng;
using namespace seng::components;
using namespace glm;

Transform::Transform(scene::Entity& e, glm::vec3 p, glm::vec3 s, glm::vec3 r) :
    BaseComponent(e)
{
  position(p);
  scale(s);
  rotation(r);
}

void Transform::position(glm::vec3 p)
{
  m_pos = p;
}

void Transform::translate(glm::vec3 pos)
{
  m_pos += pos;
}

void Transform::scale(glm::vec3 scale)
{
  scale.x = scale.x == 0.0f ? 1.0 : scale.x;
  scale.y = scale.y == 0.0f ? 1.0 : scale.y;
  scale.z = scale.z == 0.0f ? 1.0 : scale.z;
  m_scale = scale;
}

void Transform::rotation(glm::quat r)
{
  m_rotation = r;
}

void Transform::rotation(glm::vec3 euler)
{
  m_rotation = glm::quat(euler);
}

void Transform::rotate(glm::vec3 euler, CoordinateSystem ref)
{
  switch (ref) {
    case CoordinateSystem::eWorld:
      m_rotation = glm::quat(euler) * m_rotation;
      break;
    case CoordinateSystem::eLocal:
      m_rotation = m_rotation * glm::quat(euler);
      break;
  }
}

void Transform::rotate(float angle, glm::vec3 axis)
{
  m_rotation = glm::rotate(glm::quat(glm::vec3(0.0, 0.0, 0.0)), angle, axis) * m_rotation;
}

glm::vec3 Transform::forward() const
{
  return this->toMat4()[2];
}

glm::vec3 Transform::up() const
{
  return this->toMat4()[1];
}

glm::vec3 Transform::right() const
{
  return this->toMat4()[0];
}

mat4 Transform::toMat4() const
{
  return glm::translate(glm::mat4(1.0f), m_pos) *
         glm::toMat4(glm::normalize(m_rotation)) * glm::scale(glm::mat4(1.0f), m_scale);
}

std::unique_ptr<BaseComponent> Transform::createFromConfig(scene::Entity& entity,
                                                           const YAML::Node& node)
{
  glm::vec3 pos = DEFAULT_POS;
  glm::vec3 scale = DEFAULT_SCALE;
  glm::vec3 rot = DEFAULT_ROT;

  if (node["position"]) pos = node["position"].as<glm::vec3>(DEFAULT_POS);
  if (node["scale"] && node["scale"].IsScalar())
    scale = node["scale"].as<glm::vec3>(DEFAULT_SCALE);
  if (node["rotation_deg"])
    rot = glm::radians(node["rotation_deg"].as<glm::vec3>(DEFAULT_ROT));
  if (node["rotation_rad"]) rot = node["rotation_rad"].as<glm::vec3>(DEFAULT_ROT);

  return std::make_unique<Transform>(entity, pos, scale, rot);
}
