#pragma once

#include <seng/rendering/buffer.hpp>
#include <seng/rendering/mesh.hpp>
#include <seng/rendering/object_shader.hpp>
#include <seng/rendering/shader_stage.hpp>
#include <seng/scene/scene_graph.hpp>
#include <seng/time.hpp>

#include <glm/trigonometric.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <functional>
#include <string>
#include <unordered_map>

namespace seng {
class Application;

namespace rendering {
class Renderer;
class FrameHandle;
}  // namespace rendering

namespace components {
class Camera;
}

};  // namespace seng

namespace seng::scene {

/**
 * Identifies the types of events that the scene will emit:
 *
 * 1. EARLY_UPDATE: Runs before input events have been polled
 * 2. UPDATE: Main update event
 * 3. LATE_UPDATE: Runs after the scene is finished drawing.
 */
enum class SceneEvents { EARLY_UPDATE, UPDATE, LATE_UPDATE };

class SceneEventToken {
 private:
  SceneEvents event;
  uint64_t id;

  friend class Scene;
};

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
 * A scene is non-copyable and non movable.
 */
class Scene {
 public:
  using EventHandlerType = std::tuple<std::uint64_t, std::function<void(float)>>;

  Scene(Application &app);
  Scene(const Scene &) = delete;
  Scene(Scene &&) = delete;
  ~Scene();

  Scene &operator=(const Scene &) = delete;
  Scene &operator=(Scene &&) = delete;

  /**
   * Return a pointer to the main camera, if one is registered.
   */
  const components::Camera *camera() const { return mainCamera; }

  /**
   * Return a pointer to the main camera, if one is registered.
   */
  components::Camera *camera() { return mainCamera; }

  /**
   * Registers the given camera as the camera that will be used for drawing.
   */
  void setMainCamera(components::Camera *cam);

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

  /**
   * Register for the given event. The handler will receive as input th delta in
   * seconds between the current frame and the last drawn frame.
   */
  SceneEventToken listen(SceneEvents event, std::function<void(float)> cb);

  /**
   * Unregister for the given event.
   */
  void unlisten(SceneEventToken tok);

  /**
   * Fires the given event.
   */
  void fireEventType(SceneEvents event, float delta) const;

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
  components::Camera *mainCamera;
  SceneGraph sceneGraph;

  // Event dispatcher
  std::unordered_map<SceneEvents, std::vector<EventHandlerType>> eventHandlers;

  static uint64_t EVENT_INDEX;
};

};  // namespace seng::scene
