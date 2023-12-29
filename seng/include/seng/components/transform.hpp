#pragma once

#include <seng/components/base_component.hpp>
#include <seng/components/definitions.hpp>
#include <seng/components/scene_config_component_factory.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace YAML {
class Node;
};

namespace seng::scene {
class Entity;
};

namespace seng::components {

/**
 * Encodes the position, rotation and scale of an object.
 *
 * Every object has a position/rotation/scale in the coordinates of the scene
 * (which are basically the world coordinates). This class encodes said
 * transformation and allows to modify it.
 *
 * Small visualization of the coordinate system
 *
 *    y
 *    ^
 *    |
 *    |
 *    +----> x
 *   /
 *  z
 *
 * TODO: allow to build a hierarchy of transforms
 */
class Transform : public BaseComponent, public ConfigParsableComponent<Transform> {
 public:
  static constexpr glm::vec3 DEFAULT_POS = glm::vec3(0.0f);
  static constexpr glm::vec3 DEFAULT_SCALE = glm::vec3(1.0f);
  static constexpr glm::vec3 DEFAULT_ROT = glm::vec3(0.0f);

  /**
   * Create a new transform with the given position, scale and rotation
   */
  Transform(scene::Entity& entity,
            glm::vec3 pos = DEFAULT_POS,
            glm::vec3 scale = DEFAULT_SCALE,
            glm::vec3 rotation = DEFAULT_ROT);
  Transform(const Transform&) = delete;
  Transform(Transform&&) = delete;

  Transform& operator=(const Transform&) = delete;
  Transform& operator=(Transform&&) = delete;

  DECLARE_COMPONENT_ID("Transform");
  static std::unique_ptr<BaseComponent> createFromConfig(scene::Entity& entity,
                                                         const YAML::Node& node);

  /**
   * Converts this transform to a matrix that transforms the local-to-this-transform
   * coordinates to global coordinates.
   */
  glm::mat4 toMat4() const;

  /**
   * Apply a translation of the given amount along the x, y or z axes.
   */
  void translate(float x, float y, float z);

  /**
   * Apply a translation of the given vector
   */
  void translate(glm::vec3 pos);

  /**
   * Rotate the transform by the given angle (in radians) along the x, y, or z
   * axes.
   */
  void rotate(float xAngle, float yAngle, float zAngle);

  /**
   * Rotate the transform along the given axis by the given angle (in radians)
   */
  void rotate(float angle, glm::vec3 axis);

  /**
   * Set the position in the scene for this transform.
   */
  void position(float x = 0.0f, float y = 0.0f, float z = 0.0f);

  /**
   * Apply a scale to the transform along the x, y, or z axes.
   *
   * Note: zero-ing out any of the coordinates translates to a noop.
   */
  void scale(float x = 1.0f, float y = 1.0f, float z = 1.0f);

  /**
   * Sets the rotation of this transform to the given angles (in radians).
   */
  void rotation(float x = 0.0f, float y = 0.0f, float z = 0.0f);

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
   * Return the unitary vector representing the down direction of this transform
   * (which would be the local x axis)
   */
  glm::vec3 right() const;

  /**
   * Return the a vector containing the euler angles of the transform
   */
  glm::vec3 eulerAngles() const;

 private:
  glm::vec3 m_pos;
  glm::vec3 m_scale;
  glm::quat m_rotation;
};

};  // namespace seng::components
