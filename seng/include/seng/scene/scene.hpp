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

namespace rendering {
class Renderer;
class FrameHandle;
}  // namespace rendering

};  // namespace seng

namespace seng::scene {

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
  Scene(Application &app);
  Scene(const Scene &) = delete;
  Scene(Scene &&) = default;
  ~Scene();

  Scene &operator=(const Scene &) = delete;
  Scene &operator=(Scene &&) = default;


  /**
   * Parse the scene YAML from disk and set the state of this scene to the
   * parsed one.
   *
   * Scenes are searched inside the `scenePath`. Filename construction is done
   * like this: `${scenePath}/${sceneName}.yml`.
   */
  void loadFromDisk(std::string sceneName);

  /**
   * Draw the scene's contents into the currently on-going frame reprsented by the
   * given FrameHandle.
   */
  void draw(const rendering::FrameHandle &handle);

 private:
  Application *app;
  rendering::Renderer *renderer;

  // Global descriptor layout
  vk::raii::DescriptorSetLayout globalDescriptorSetLayout;

  std::unordered_map<std::string, rendering::ShaderStage> stages;
  std::unordered_map<std::string, rendering::ObjectShader> shaders;
  // TODO: map<string, ObjectShader::Instance> shaderInstances
  std::unordered_map<std::string, rendering::Mesh> meshes;

  // Scene graph
  SceneGraph sceneGraph;
};

};  // namespace seng::scene
