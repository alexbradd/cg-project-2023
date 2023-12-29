#include <seng/components/transform.hpp>
#include <seng/yaml_utils.hpp>

// clang-format off
#include <glm/gtx/quaternion.hpp>
// clang-format on
#include <yaml-cpp/yaml.h>

using namespace seng;
using namespace seng::components;
using namespace glm;

Transform::Transform(scene::Entity& e, vec3 pos, vec3 scale, vec3 rotation) :
    BaseComponent(e)
{
  setPos(pos.x, pos.y, pos.z);
  setScale(scale.x, scale.y, scale.x);
  setRotation(rotation.x, rotation.y, rotation.z);
}

void Transform::setPos(float x, float y, float z)
{
  m_pos = vec3(x, y, z);
}

void Transform::translate(float x, float y, float z)
{
  this->translate(vec3(x, y, z));
}

void Transform::translate(glm::vec3 pos)
{
  m_pos += pos;
}

void Transform::setScale(float x, float y, float z)
{
  x = x == 0.0f ? 1.0 : x;
  y = y == 0.0f ? 1.0 : y;
  z = z == 0.0f ? 1.0 : z;
  m_scale = vec3(x, y, z);
}

void Transform::setRotation(float xAngle, float yAngle, float zAngle)
{
  // Quaternion can be created from euler angles with the Pitch-Yaw-Roll order
  m_rotation = glm::quat(vec3(yAngle, zAngle, xAngle));
}

void Transform::rotate(float xAngle, float yAngle, float zAngle)
{
  quat dQ = quat(vec3(0.0f, yAngle, 0.0f)) * quat(vec3(xAngle, 0.0f, 0.0f)) *
            quat(vec3(0.0f, 0.0f, zAngle));
  m_rotation = m_rotation * dQ;
}

void Transform::rotate(float angle, vec3 axis)
{
  m_rotation = m_rotation * glm::rotate(quat(1.0f, 0.0f, 0.0f, 0.0f), angle, axis);
}

vec3 Transform::forward() const
{
  return vec3(this->toMat4()[2]);
}

vec3 Transform::up() const
{
  return vec3(this->toMat4()[1]);
}

vec3 Transform::right() const
{
  return vec3(this->toMat4()[0]);
}

mat4 Transform::toMat4() const
{
  return glm::translate(glm::mat4(1.0f), m_pos) * glm::toMat4(m_rotation) *
         glm::scale(glm::mat4(1.0f), m_scale);
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
