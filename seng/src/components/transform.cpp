#include <seng/components/transform.hpp>
#include <seng/yaml_utils.hpp>

#include <yaml-cpp/yaml.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

using namespace seng;

Transform::Transform(Entity& e, glm::vec3 p, glm::vec3 s, glm::vec3 r) : BaseComponent(e)
{
  position(p);
  scale(s);
  rotation(r);
  m_changes = POSITION | ROTATION | SCALE;
}

void Transform::position(glm::vec3 p)
{
  m_pos = p;
  m_changes |= POSITION | CHANGE_TRACKER;
}

void Transform::translate(glm::vec3 pos)
{
  position(m_pos + pos);
}

void Transform::scale(glm::vec3 scale)
{
  scale.x = scale.x == 0.0f ? 1.0 : scale.x;
  scale.y = scale.y == 0.0f ? 1.0 : scale.y;
  scale.z = scale.z == 0.0f ? 1.0 : scale.z;
  m_scale = scale;
  m_changes |= SCALE | CHANGE_TRACKER;
}

void Transform::rotation(glm::quat r)
{
  m_rotation = r;
  m_changes |= ROTATION | CHANGE_TRACKER;
}

void Transform::rotation(glm::vec3 euler)
{
  rotation(glm::quat(euler));
}

void Transform::rotate(glm::vec3 euler, CoordinateSystem ref)
{
  switch (ref) {
    case CoordinateSystem::eWorld:
      rotation(glm::quat(euler) * m_rotation);
      break;
    case CoordinateSystem::eLocal:
      rotation(m_rotation * glm::quat(euler));
      break;
  }
}

void Transform::rotate(float angle, glm::vec3 axis)
{
  rotation(glm::rotate(glm::quat(glm::vec3(0.0, 0.0, 0.0)), angle, axis) * m_rotation);
}

void Transform::lookAt(const Transform& other, glm::vec3 upDirection)
{
  glm::vec3 fwd = glm::normalize(other.m_pos - this->m_pos);
  rotation(glm::quatLookAtLH(fwd, upDirection));
}

const glm::mat4& Transform::rotationMatrix() const
{
  if (m_changes & ROTATION) {
    m_rotMat = glm::toMat4(glm::normalize(m_rotation));
    m_rotMat[2] *= -1;
    m_changes &= ~ROTATION;
  }
  return m_rotMat;
}

glm::vec3 Transform::forward() const
{
  return -rotationMatrix()[2];
}

glm::vec3 Transform::up() const
{
  return rotationMatrix()[1];
}

glm::vec3 Transform::right() const
{
  return rotationMatrix()[0];
}

glm::mat4 Transform::toMat4() const
{
  if (m_changes & ~CHANGE_TRACKER) {
    m_localToWorld = glm::translate(glm::mat4(1.0f), m_pos) * rotationMatrix() *
                     glm::scale(glm::mat4(1.0f), m_scale);
    m_changes &= CHANGE_TRACKER;
  }
  return m_localToWorld;
}

DEFINE_CREATE_FROM_CONFIG(Transform, entity, node)
{
  glm::vec3 pos = DEFAULT_POS;
  glm::vec3 scale = DEFAULT_SCALE;
  glm::vec3 rot = DEFAULT_ROT;

  if (node["position"]) pos = node["position"].as<glm::vec3>(DEFAULT_POS);
  if (node["scale"]) scale = node["scale"].as<glm::vec3>(DEFAULT_SCALE);
  if (node["rotation_deg"])
    rot = glm::radians(node["rotation_deg"].as<glm::vec3>(DEFAULT_ROT));
  if (node["rotation_rad"]) rot = node["rotation_rad"].as<glm::vec3>(DEFAULT_ROT);

  return std::make_unique<Transform>(entity, pos, scale, rot);
}
