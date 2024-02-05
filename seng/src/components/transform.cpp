#include <seng/components/transform.hpp>
#include <seng/log.hpp>
#include <seng/scene/entity.hpp>
#include <seng/scene/scene.hpp>
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

#include <optional>

using namespace seng;

Transform::Transform(Entity& e,
                     std::optional<std::string> parentName,
                     glm::vec3 p,
                     glm::vec3 s,
                     glm::vec3 r) :
    BaseComponent(e)
{
  if (parentName.has_value()) {
    auto parent = e.scene().findByName(*parentName);
    if (parent == e.scene().entities().end()) {
      seng::log::warning("Parent does not exists, defaulting to null");
      m_parent = nullptr;
    } else {
      seng::log::dbg("Parenting to {}", *parentName);
      m_parent = parent->transform();
      m_parent->m_children.insert(this);
    }
  } else
    m_parent = nullptr;

  position(p);
  scale(s);
  rotation(r);
  m_changes = POSITION | ROTATION | SCALE | CHANGE_TRACKER;
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
  scale.x = scale.x <= 0.0f ? 1.0 : scale.x;
  scale.y = scale.y <= 0.0f ? 1.0 : scale.y;
  scale.z = scale.z <= 0.0f ? 1.0 : scale.z;
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
  glm::vec3 thisV, otherV;

  thisV = transformToWorld(m_pos);
  otherV = other.transformToWorld(other.m_pos);

  glm::vec3 fwd = otherV - thisV;
  if (glm::length2(fwd) > 0.0f) {
    fwd = glm::normalize(fwd);
    rotation(glm::quatLookAtLH(fwd, upDirection));

    // I really do not know why this is necessary, I found it in three.js's
    // source code and it works, so I don't ask qustions
    if (m_parent != nullptr) rotation(glm::inverse(m_parent->m_rotation) * m_rotation);
  }
}

const glm::mat4& Transform::rotationMatrix() const
{
  if (m_changes & ROTATION) {
    m_rotMat = glm::toMat4(glm::normalize(m_rotation));
    m_changes &= ~ROTATION;
  }
  return m_rotMat;
}

glm::vec3 Transform::forward() const
{
  return rotationMatrix()[2];
}

glm::vec3 Transform::up() const
{
  return rotationMatrix()[1];
}

glm::vec3 Transform::right() const
{
  return rotationMatrix()[0];
}

const glm::mat4& Transform::localMatrix() const
{
  if ((m_changes & ~CHANGE_TRACKER)) {
    m_local = glm::translate(glm::mat4(1.0f), m_pos) * rotationMatrix() *
              glm::scale(glm::mat4(1.0f), m_scale);
    m_changes &= CHANGE_TRACKER;
  }
  return m_local;
}

glm::mat4 Transform::worldMartix() const
{
  if (m_parent != nullptr)
    return m_parent->worldMartix() * localMatrix();
  else
    return localMatrix();
}

glm::vec3 Transform::transformToWorld(const glm::vec3& v) const
{
  if (m_parent != nullptr)
    return m_parent->worldMartix() * glm::vec4(v, 1.0f);
  else
    return v;
}

Transform::~Transform()
{
  if (!m_children.empty()) {
    seng::log::dbg("Reparenting children to nearest parent");
    for (auto c : m_children) c->m_parent = m_parent;
  }
  if (m_parent != nullptr) m_parent->m_children.erase(this);
}

DEFINE_CREATE_FROM_CONFIG(Transform, entity, node)
{
  glm::vec3 pos = DEFAULT_POS;
  glm::vec3 scale = DEFAULT_SCALE;
  glm::vec3 rot = DEFAULT_ROT;
  std::optional<std::string> parent = std::nullopt;

  if (node["parent"]) parent = node["parent"].as<std::string>();
  if (node["position"]) pos = node["position"].as<glm::vec3>(DEFAULT_POS);
  if (node["scale"]) scale = node["scale"].as<glm::vec3>(DEFAULT_SCALE);
  if (node["rotation_deg"])
    rot = glm::radians(node["rotation_deg"].as<glm::vec3>(DEFAULT_ROT));
  if (node["rotation_rad"]) rot = node["rotation_rad"].as<glm::vec3>(DEFAULT_ROT);

  return std::make_unique<Transform>(entity, parent, pos, scale, rot);
}
