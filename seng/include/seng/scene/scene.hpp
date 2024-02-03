#pragma once

#include <seng/hook.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/scene/direct_light.hpp>
#include <seng/scene/entity.hpp>
#include <seng/time.hpp>

#include <glm/trigonometric.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <list>
#include <memory>
#include <string>
#include <unordered_map>

namespace YAML {
class Node;
}

namespace seng {
class Application;
class Camera;
class MeshRenderer;

namespace rendering {
class Renderer;
class FrameHandle;
class CommandBuffer;
}  // namespace rendering

/**
 * A Scene is the container for all resources currently loaded that are not
 * strictly related to the low-level push-frame-to-display, e.g. meshes, materials,
 * entities and other objects.
 *
 * The "stuff that the user can interact with", i.e. entities, are organized in a
 * scene graph. For more info on those, refer to the specific classes.
 *
 * Scene provides some hooks into its update cycle. One can register to these
 * hooks through the provided registrars (see `seng/hook.hpp` for more info).
 * The three most important hooks for game logic provided are:
 *
 * - `onEarlyUpdate`: runs first thing in the update cycle
 * - `onUpdate`: runs just before scene drawing
 * - `onLateUpdate`: runs last thing in the update cycle
 *
 * Also other hooks into other aspects of a scene's life (e.g. drawing) are exposed.
 * For more info referer to the specific hooks' documentation.
 *
 * A scene is non-copyable and non movable.
 */
class Scene {
 public:
  /// Typedef for the collection holding all entities in the scene
  using EntityList = std::list<Entity>;

  Scene(Application &app);
  Scene(const Scene &) = delete;
  Scene(Scene &&) = delete;
  ~Scene();

  Scene &operator=(const Scene &) = delete;
  Scene &operator=(Scene &&) = delete;

  /**
   * Parse the scene YAML corresponding to the scene with the given name from disk
   * and create a new instance maching it.
   *
   * Scenes are searched inside the `scenePath`. Filename construction is done
   * like this: `${scenePath}/${sceneName}.yml`.
   */
  static std::unique_ptr<Scene> loadFromDisk(Application &app, std::string sceneName);

  /**
   * Find the first Entity in the scene graph with the given name. If no such
   * entity can be found, a past-the-end iterator is returned.
   *
   * N.B.: searching for an entity is linear in the number of entites in the
   * scene graph, so make sure to cache the iterator (it is safe to do
   * so since they are guaranteed to not be invaliedated w.r.t. scene graph changes).
   */
  EntityList::const_iterator findByName(const std::string &name) const;

  /**
   * Find the first Entity in the scene graph with the given name. If no such
   * entity can be found, a past-the-end iterator is returned.
   *
   * N.B.: searching for an entity is linear in the number of entites in the
   * scene graph, so make sure to cache the iterator (it is safe to do
   * so since they are guaranteed to not be invaliedated w.r.t. scene graph changes).
   */
  EntityList::iterator findByName(const std::string &name);

  /**
   * Collect all references to instances with the given name in a vector and
   * return them.
   *
   * N.B.: searching for an entity is linear in the number of entites in the
   * scene graph, so make sure to cache the iterator (it is safe to do
   * so since they are guaranteed to not be invaliedated w.r.t. scene graph changes).
   */
  std::vector<const Entity *> findAllByName(const std::string &name) const;

  /**
   * Collect all references to instances with the given name in a vector and
   * return them.
   *
   * N.B.: searching for an entity is linear in the number of entites in the
   * scene graph, so make sure to cache the iterator (it is safe to do
   * so since they are guaranteed to not be invaliedated w.r.t. scene graph changes).
   */
  std::vector<Entity *> findAllByName(const std::string &name);

  /**
   * Create a new Entity into the scene graph with the given name and return a
   * pointer to it.
   */
  Entity *newEntity(std::string name);

  /**
   * Delete the corresponding Entity from the scene graph.
   *
   * N.B. This incurs linear search time.
   */
  void removeEntity(const Entity *e);

  /**
   * Delete the Entity with the given name from the scene graph.
   *
   * N.B. This incurs linear search time.
   */
  void removeEntity(const std::string &name);

  /**
   * Delete the Entity referenced by the given iterator
   */
  void removeEntity(EntityList::const_iterator it);

  /**
   * Destroy all entities in a scene
   */
  void removeAllEntities();

  /**
   * Return a pointer to the main camera, if one is registered.
   */
  const Camera *mainCamera() const { return m_mainCamera; }

  /**
   * Return a pointer to the main camera, if one is registered.
   */
  Camera *mainCamera() { return m_mainCamera; }

  /**
   * Registers the given camera as the camera that will be used for drawing.
   */
  void mainCamera(Camera *cam);

  /// Get the scene's ambient color
  glm::vec4 ambientColor() const { return m_ambient; }

  /// Set a new ambient color
  void ambientColor(glm::vec4 color) { m_ambient = color; }

  /// Get a const reference to the scene's direct light
  const DirectLight &light() const { return m_directLight; }

  /// Get a reference to the scene's direct light
  DirectLight &light() { return m_directLight; }

  /**
   * Registrar for the "earlyUpdate" hook
   *
   * This hook is executed at the earliest point in the update cycle
   */
  HookRegistrar<float> &onEarlyUpdate() { return m_earlyUpdate.registrar(); }

  /**
   * Registrar for the "update" hook
   *
   * This hook is executed right in the middle of the scene's update cycle, just
   * before drawing.
   */
  HookRegistrar<float> &onUpdate() { return m_update.registrar(); }

  /**
   * Registrar for the "lateUpdate" hook
   *
   * This hook is executed last thing in the update cycle
   */
  HookRegistrar<float> &onLateUpdate() { return m_lateUpdate.registrar(); }

  /**
   * Get the registrar for the "shaderInstanceDraw" relative to the instance with
   * the given name.
   *
   * This hook gets executed when a shader instance is ready for drawing
   * (pipeline and descriptors bound).
   */
  HookRegistrar<const rendering::CommandBuffer &> &onShaderInstanceDraw(
      const std::string &instance);

  /**
   * Draw the scene's contents into the currently on-going frame reprsented by the
   * given FrameHandle.
   */
  void draw(const rendering::FrameHandle &handle);

  /**
   * Update the current scene
   */
  void update(Duration frameTime, const rendering::FrameHandle &handle);

 private:
  Application *m_app;
  rendering::Renderer *m_renderer;

  // Light information
  glm::vec4 m_ambient;
  DirectLight m_directLight;

  // Hooks
  Hook<float> m_earlyUpdate;
  Hook<float> m_update;
  Hook<float> m_lateUpdate;
  std::unordered_map<std::string, Hook<const rendering::CommandBuffer &>> m_renderers;

  // Scene graph
  Camera *m_mainCamera;
  EntityList m_entities;

  void parseEntity(const YAML::Node &node);
};

};  // namespace seng
