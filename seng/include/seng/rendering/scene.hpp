#pragma once

#include <seng/camera.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/mesh.hpp>
#include <seng/rendering/object_shader.hpp>
#include <seng/rendering/shader_stage.hpp>

#include <glm/trigonometric.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace seng {
class Application;
};  // namespace seng

namespace seng::rendering {

class Renderer;
class FrameHandle;

/**
 * A Scene is the container for all resources currently loaded that are not
 * strictly related to the low-level push-frame-to-display, e.g. meshes, materials,
 * entities and other objects.
 *
 * A scene's resources can be divided roughly in two:
 *
 * 1. Stuff necessary for shading (meshes, shader stages, object shaders and shader
 *    instances)
 * 2. Stuff that the user can move around an interact with (entities and the camera)
 *
 * The "stuff that the user can interact with", i.e. entities, are organized in a
 * scene graph. For more info on those, refer to the specific classes.
 *
 * A scene is non-copyable, only movable.
 */
class Scene {
 public:
  Scene(const Scene &) = delete;
  Scene(Scene &&) = default;
  ~Scene();

  Scene &operator=(const Scene &) = delete;
  Scene &operator=(Scene &&) = default;

  // Accessors
  const Camera &mainCamera() const { return camera; }
  Camera &mainCamera() { return camera; }

  /**
   * Factory that parses the scene YAML from disk and allocates it.
   *
   * Scenes are searched inside the `scenePath`. Filename construction is done
   * like this: `${scenePath}/${sceneName}.yml`.
   */
  static Scene loadFromDisk(const Application &app,
                            std::string sceneName,
                            float cameraAspectRatio);

  /**
   * Draw the scene's contents into the currently on-going frame reprsented by the
   * given FrameHandle.
   */
  void draw(const FrameHandle &handle);

 private:
  /**
   * Utility struct used in parsing camera info from configuration. Mirrors the
   * constructor from `seng::Camera`.
   */
  struct CameraParams {
    float aspectRatio;
    float near = 0.1f;
    float far = 1000.0f;
    float fov = glm::radians(45.0f);
  };

  Renderer *renderer;

  // Global descriptor layout
  vk::raii::DescriptorSetLayout globalDescriptorSetLayout;

  std::unordered_map<std::string, ShaderStage> stages;
  std::unordered_map<std::string, ObjectShader> shaders;
  // TODO: map<string, ObjectShader::Instance> shaderInstances
  std::unordered_map<std::string, Mesh> meshes;

  // Scene graph
  Camera camera;

  /**
   * Private constructor. Users of the class should go through the `loadFromDisk()`
   * factory method.
   */
  Scene(const Application &app,
        const CameraParams &cameraParams,
        const std::unordered_set<std::string> &stageNames,
        const std::unordered_set<std::string> &shaderNames,
        const std::unordered_set<std::string> &meshNames);
};

};  // namespace seng::rendering
