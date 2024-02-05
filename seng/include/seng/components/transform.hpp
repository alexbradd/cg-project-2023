#pragma once

#include <seng/components/base_component.hpp>
#include <seng/components/definitions.hpp>
#include <seng/components/scene_config_component_factory.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <optional>
#include <unordered_set>

namespace YAML {
class Node;
};

namespace seng {
class Entity;

/**
 * Enumeration representing the coordinate system to use in caluclations.
 */
enum struct CoordinateSystem { eLocal, eWorld };

/**
 * Encodes the position, rotation and scale of an object.
 *
 * Every Entity in a scene has a Transform component attached. Every Transform
 * can have a parent, which allows one to apply position, rotation and scale
 * hierarchically.
 *
 * The position/rotation/scale retrieved by use of the accessor methods are all
 * relative to the local space.
 *
 * The coordinate system used is a left-handed Y-up system. Small visualization:
 *
 *    y
 *    ^
 *    | z      Positive rotation:
 *    |/         clockwise around the axis or rotation
 *    +----> x
 *
 */
class Transform : public BaseComponent, public ConfigParsableComponent<Transform> {
 public:
  static constexpr glm::vec3 DEFAULT_POS = glm::vec3(0.0f);
  static constexpr glm::vec3 DEFAULT_SCALE = glm::vec3(1.0f);
  static constexpr glm::vec3 DEFAULT_ROT = glm::vec3(0.0f);

  /**
   * Create a new transform with the given position, scale and rotation parented
   * to the transform attached to the entity with the given name.
   */
  Transform(Entity& entity,
            std::optional<std::string> parentName = std::nullopt,
            glm::vec3 pos = DEFAULT_POS,
            glm::vec3 scale = DEFAULT_SCALE,
            glm::vec3 rotation = DEFAULT_ROT);
  Transform(const Transform&) = delete;
  Transform(Transform&&) = delete;
  ~Transform();

  Transform& operator=(const Transform&) = delete;
  Transform& operator=(Transform&&) = delete;

  DECLARE_COMPONENT_ID("Transform");
  DECLARE_CREATE_FROM_CONFIG();

  /**
   * Returns the local-space matrix of this transform
   */
  const glm::mat4& localMatrix() const;

  /**
   * Returns the world-space matrix of this transform
   */
  glm::mat4 worldMartix() const;

  /**
   * Transform the given vector from local coordinates to global coordinates
   */
  glm::vec3 transformToWorld(const glm::vec3& v) const;

  /**
   * Set the position in the scene for this transform.
   */
  void position(glm::vec3 pos);

  /**
   * Return the transfrom's position.
   */
  glm::vec3 position() const { return m_pos; }

  /**
   * Apply a translation of the given vector.
   */
  void translate(glm::vec3 pos);

  /**
   * Sets the rotation of this transform to the given euler angles (in radians).
   * The order of angles should be Pitch, Yaw and Roll (XYZ).
   */
  void rotation(glm::vec3 euler);

  /**
   * Sets the rotation of this transform to the given quaternion.
   */
  void rotation(glm::quat quat);

  /**
   * Return the current pitch (rotation alogn the X axis in radians).
   */
  float pitch() const { return glm::pitch(m_rotation); }

  /**
   * Return the current yaw (rotation alogn the y axis in radians).
   */
  float yaw() const { return glm::yaw(m_rotation); }

  /**
   * Return the current roll (rotation along the z axis in radians).
   */
  float roll() const { return glm::roll(m_rotation); }

  /**
   * Return a vector containing the euler angles of the transform. The order of
   * angler is Pitch, Yaw and Roll (XYZ).
   */
  glm::vec3 eulerAngles() const { return glm::eulerAngles(m_rotation); }

  /**
   * Return a quaternion encoding the transforms rotation.
   */
  glm::quat quaternion() const { return m_rotation; }

  /**
   * Rotate the transform by the given angles (in radians) along the x, y, or z
   * axes in the given coordinate system.
   */
  void rotate(glm::vec3 euler, CoordinateSystem ref = CoordinateSystem::eLocal);

  /**
   * Rotate the transform along the given axis by the given angle (in radians).
   */
  void rotate(float angle, glm::vec3 axis);

  /**
   * Return the transfrom's position.
   */
  glm::vec3 scale() const { return m_scale; }

  /**
   * Apply a scale to the transform along the x, y, or z axes.
   *
   * Note: zero-ing out any coordinate translates to no scaling along that
   * axis.
   */
  void scale(glm::vec3 scale);

  /**
   * Rotates the transform so the forward vector points at the given Transform's
   * current position. The transform is rotated such that its `up` direction
   * matches the given `upDirection` (by default it is `worldUp`).
   */
  void lookAt(const Transform& other, glm::vec3 upDirection = Transform::worldUp());

  /**
   * Queries if the `changed` flag is set.
   *
   * This flag becomes true if the transform has been changed
   * at any time before this call and noone has explicitly cleared it.
   */
  bool changed() const { return m_changes & CHANGE_TRACKER; }

  /**
   * Clears the `changed` flag.
   */
  void clearChanged() { m_changes = m_changes | CHANGE_TRACKER; }

  /**
   * Return the unitary vector representing the forward direction of this transform
   * (which would be the local z axis)
   */
  glm::vec3 forward() const;

  /**
   * Return the unitary vector representing the up direction of this transform
   * (which would be the local y axis)
   */
  glm::vec3 up() const;

  /**
   * Return the unitary vector representing the right direction of this transform
   * (which would be the local x axis)
   */
  glm::vec3 right() const;

  /**
   * Return the unitary vector representing the world space forward direction
   * (which would be the global z axis)
   */
  static constexpr glm::vec3 worldForward() { return glm::vec3(0.0f, 0.0f, 1.0f); }

  /**
   * Return the unitary vector representing the world space up direction
   * (which would be the global y axis)
   */
  static constexpr glm::vec3 worldUp() { return glm::vec3(0.0f, 1.0f, 0.0f); }

  /**
   * Return the unitary vector representing the world space right direction
   * (which would be the global x axis)
   */
  static constexpr glm::vec3 worldRight() { return glm::vec3(1.0f, 0.0f, 0.0f); }

 private:
  static constexpr uint32_t POSITION = 0x00000001;
  static constexpr uint32_t ROTATION = 0x00000002;
  static constexpr uint32_t SCALE = 0x00000004;
  static constexpr uint32_t CHANGE_TRACKER = 0x80000000;

  Transform* m_parent;
  std::unordered_set<Transform*> m_children;

  glm::vec3 m_pos;
  glm::vec3 m_scale;
  glm::quat m_rotation;

  mutable uint32_t m_changes;
  mutable glm::mat4 m_rotMat;
  mutable glm::mat4 m_local;

  const glm::mat4& rotationMatrix() const;
};

REGISTER_TO_CONFIG_FACTORY(Transform);

};  // namespace seng
