#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include <seng/components/base_component.hpp>
#include <seng/components/definitions.hpp>
#include <seng/components/scene_config_component_factory.hpp>

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>

#include <vector>

namespace seng {

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
 * There should be one main camera per scene, but there can be multiple non-main
 * ones. If there are multiple cameras, only the one set as main will render.
 * If multiple main cameras are set as main, it is undefined which will be
 * registered as main in the end.
 *
 * One particular thing to note: the camera is oriented along the -z axis,
 * meaning that translating along `forward()` moves the camera backwards.
 */
class Camera : public BaseComponent, public ConfigParsableComponent<Camera> {
 public:
  static constexpr float DEFAULT_NEAR = 0.1f;
  static constexpr float DEFAULT_FAR = 1000.0f;
  static constexpr float DEFAULT_FOV = glm::radians(45.0f);
  static constexpr bool DEFAULT_MAIN = false;

  Camera(scene::Entity& entity,
         float near = DEFAULT_NEAR,
         float far = DEFAULT_FAR,
         float fov = DEFAULT_FOV,
         bool main = DEFAULT_MAIN);
  Camera(const Camera&) = delete;
  Camera(Camera&&) = delete;
  ~Camera();

  Camera& operator=(const Camera&) = delete;
  Camera& operator=(Camera&&) = delete;

  DECLARE_COMPONENT_ID("Camera");
  static std::unique_ptr<BaseComponent> createFromConfig(scene::Entity& e,
                                                         const YAML::Node&);

  // Overrides
  void initialize() override;

  // Getters
  float aspectRatio() const { return _aspectRatio; }
  float nearPlane() const { return _near; }
  float farPlane() const { return _far; }
  float fov() const { return _fov; }

  static const std::vector<Camera*>& allCameras() { return cameras; }

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
  bool registerAsMain;
  float _aspectRatio;
  float _near;
  float _far;
  float _fov;

  static std::vector<Camera*> cameras;

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
