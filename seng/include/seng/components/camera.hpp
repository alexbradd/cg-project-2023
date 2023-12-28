#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include <seng/components/base_component.hpp>
#include <seng/components/definitions.hpp>
#include <seng/components/scene_config_component_factory.hpp>

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>

namespace seng {
class Application;

namespace scene {
class Entity;
};  // namespace scene

namespace components {

/**
 * Component representing the device through which we view the scene.
 *
 * Basically, it is a container for the camera parameters (near and far planes,
 * aspect ratio and fov) with added utilities, like calculating the matrices.
 *
 * There should be one camera per scene. If there are multiple, only one will
 * render. Which one is not defined.
 *
 * One particular thing to note: the camera is oriented along the -z axis,
 * meaning that translating along `forward()` moves the camera backwards.
 */
class Camera : public BaseComponent, public ConfigParsableComponent<Camera> {
 public:
  Camera(Application& app,
         scene::Entity& entity,
         float near = 0.1f,
         float far = 1000.0f,
         float fov = glm::radians(45.0f));
  Camera(const Camera&) = delete;
  Camera(Camera&&) = default;

  Camera& operator=(const Camera&) = delete;
  Camera& operator=(Camera&&) = default;

  DECLARE_COMPONENT_ID("Camera");
  static std::unique_ptr<BaseComponent> createFromConfig(Application& a,
                                                         scene::Entity& e,
                                                         const YAML::Node&);

  // Overrides
  void initialize() override;

  // Getters
  float aspectRatio() const { return _aspectRatio; }
  float nearPlane() const { return _near; }
  float farPlane() const { return _far; }
  float fov() const { return _fov; }

  // Setters
  void nearPlane(float near);
  void farPlane(float far);
  void fov(float fov);

  /**
   * Returns the projection matrix for this camera.
   *
   * Note: calling this function repeadetely does not recompute each time the
   * matrix. The matrix is recomputed only if one of the parameters have changed.
   */
  glm::mat4 projectionMatrix() const;

  /**
   * Returns the view matrix for this camera.
   */
  glm::mat4 viewMatrix() const;

 private:
  float _aspectRatio;
  float _near;
  float _far;
  float _fov;

  /**
   * Run on window resize. Updates the aspect ratio
   */
  void resize(int width, int height);

  // For caching
  mutable bool _projectionDirty = true;
  mutable glm::mat4 _projection;
};

};  // namespace components
};  // namespace seng
