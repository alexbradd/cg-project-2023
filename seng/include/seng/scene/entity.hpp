#pragma once

#include <seng/components/component_ptr.hpp>
#include <seng/components/transform.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace seng {
class Application;
class Scene;

/**
 * An entity in a scene's scene graph.
 *
 * An entity is a "named container" of Component instances. An entity has an
 * ever-increasing unique ID associated to it.
 *
 * Every entity has always at least one component: a Transform. The entity's
 * transform gets "special treatment" by having its own bespoke accessor
 * (`transform`). The main reason for this is that the most frequent change
 * of an Entity's state will be to its Transform, so reducing lookup cost is
 * important.
 *
 * It owns all the components attached to it, provides lookup via iterators and
 * insertion/deletion modifiers. Other components that wish to store a
 * reference to another attached Component should store a raw pointer (don't
 * worry, it is guaranteed to be valid for the whole lifetime of the Component,
 * unless some other Component deletes that instance).
 */
class Entity {
 public:
  /// Alias for a vector of unique_ptr to components
  using ComponentList = std::vector<ComponentPtr>;

  /// Alias for the Component store
  using ComponentMap = std::unordered_map<std::string, ComponentList>;

 public:
  Entity(const Entity&) = delete;
  Entity(Entity&&) = default;

  Entity& operator=(const Entity&) = delete;
  Entity& operator=(Entity&&) = default;
  ~Entity();

  /// Equality operator overload. Since entities have unique IDs in the scene,
  /// we simply compare the ids.
  friend bool operator==(const Entity& lhs, const Entity& rhs)
  {
    return lhs.m_id == rhs.m_id;
  }

  /// Inequality operator, defined in terms of the equality operator
  friend bool operator!=(const Entity& lhs, const Entity& rhs) { return !(lhs == rhs); }

  // Accessors
  const Application& application() const { return *m_app; }
  Application& application() { return *m_app; }

  /// Return the scene this entity is instantiated in
  const Scene& scene() const { return *m_scene; }
  /// Return the scene this entity is instantiated in
  Scene& scene() { return *m_scene; }

  const uint64_t& id() const { return m_id; }
  const std::string& name() const { return m_name; }
  Transform* transform() const { return m_transform.sureGet<Transform>(); }

  /**
   * Return a reference to a vector of owned pointers to
   * Components of type T attached to the Entity. If no component of the given
   * type can be found, an empty vector is returned.
   *
   * IMPORTANT: please note that modifications to the Component list may
   * invalidate any iterators for the returned vectors. If one wants to keep
   * a reference to one or more Component, cache the pointers
   */
  template <typename T, typename = IfComponent<T>>
  const ComponentList& componentsOfType() const
  {
    auto it = m_components.find(T::componentId());
    if (it == m_components.end()) return EMPTY_VECTOR;
    return it->second;
  }

  /**
   * Create a new component of type T in-place. This function will add the
   * reference to this Entity as the first argument to the called constructor.
   */
  template <typename T, typename = IfComponent<T>, typename... Args>
  void emplaceComponent(Args&&... args)
  {
    m_components[T::componentId()].emplace_back(*this, std::make_unique<T>(args...));
  }

  /**
   * Destroy the component of type T stored at the given pointer location.
   */
  template <typename T, typename = IfComponent<T>>
  void removeComponent(const BaseComponent* comp_ptr)
  {
    auto it = m_components.find(T::componentId());
    removeWithIterByPtr(it, comp_ptr);
  }

 private:
  Application* m_app;
  Scene* m_scene;
  uint64_t m_id;
  std::string m_name;
  ComponentPtr m_transform;
  ComponentMap m_components;

  static uint64_t INDEX_COUNTER;
  static const ComponentList EMPTY_VECTOR;

  void transform(ComponentPtr&& transform);
  void removeWithIterByPtr(ComponentMap::iterator it, const BaseComponent* ptr);
  bool checkAndWarnCompPtr(ComponentPtr& ptr);

  /**
   * Constructor for a new entity with the given name and position in the origin.
   * Private since users shoud use the appropriate method in SceneGraph.
   */
  Entity(Application& app, Scene& scene, std::string name);

  /**
   * Private, untyped implementation of insertion
   */
  void untypedInsert(const ComponentIdType& id, ComponentPtr&& cmp);

  friend class Scene;
};

};  // namespace seng

namespace std {

// Template instantiation for hash<>, so that we can use entities in sets and maps
template <>
struct hash<seng::Entity> {
  std::size_t operator()(const seng::Entity& entity) const
  {
    hash<uint64_t> hasher;
    return hasher(entity.id());
  }
};

};  // namespace std
