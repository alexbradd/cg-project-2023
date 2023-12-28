#pragma once

#include <seng/components/base_component.hpp>
#include <seng/components/utils.hpp>
#include <seng/transform.hpp>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace seng::components {
class BaseComponent;
};  // namespace seng::components

namespace seng::scene {

/**
 * An entity in a scene's scene graph.
 *
 * An entity is a "named container" of Component instances. An entities name
 * should be unique inside the scene it is defined in.
 *
 * Every entity has always at least one component: a Transform. The entities
 * transform gets "special treatment" by having its own bespoke accessor
 * (`getTransform`). The main reason for this is that the most frequent change
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
  using ComponentList = std::vector<std::unique_ptr<components::BaseComponent>>;

  /// Alias for the Component store
  using ComponentMap = std::unordered_map<std::string, ComponentList>;

 public:
  /**
   * Constructor for a new entity with the given name and position in the origin
   *
   * Note: one should never create an Entity directly as it can bread component
   * attaching, but instead go through the scene graph.
   */
  Entity(std::string name);

  Entity(const Entity&) = delete;
  Entity(Entity&&) = default;

  Entity& operator=(const Entity&) = delete;
  Entity& operator=(Entity&&) = default;

  /// Equality operator overload. Since entities have unique names in the scene,
  /// we simply compare the names.
  friend bool operator==(const Entity& lhs, const Entity& rhs)
  {
    return lhs.name == rhs.name;
  }

  /// Inequality operator, defined in terms of the equality operator
  friend bool operator!=(const Entity& lhs, const Entity& rhs) { return !(lhs == rhs); }

  // Accessors
  const std::string& getName() const { return name; }
  const std::unique_ptr<Transform>& getTransform() const { return transform; }

  /**
   * Return an iterator to the start of a vector of owned pointers to
   * Components of type T attached to the Entity. If no component of the given
   * type can be found, nullopt is returned.
   *
   * IMPORTANT: please note that modifications to the attached Component list may
   * invalidate the returned iterator. If for some reason one wants to keep the
   * iterator, it is advised to copy the needed range using ComponentList's range
   * constructor.
   */
  template <typename T>
  std::optional<ComponentList::const_iterator> componentsOfTypeBegin() const
  {
    ASSERT_SUBCLASS_OF_COMPONENT(T);

    auto it = components.find(T::componentId());
    if (it == components.end()) return std::nullopt;
    return std::make_optional(it->second.begin());
  }

  /**
   * Return a past-the-end iterator of a vector of owned pointers to
   * Components of type T attached to the Entity. If no component of the given
   * type can be found, nullopt is returned.
   *
   * IMPORTANT: see `componentsOfTypeBegin` for considerations.
   */
  template <typename T>
  std::optional<ComponentList::const_iterator> componentsOfTypeEnd() const
  {
    ASSERT_SUBCLASS_OF_COMPONENT(T);

    auto it = components.find(T::componentId());
    if (it == components.end()) return std::nullopt;
    return std::make_optional(it->second.end());
  }

  /**
   * Create a new component of type T in-place.
   */
  template <typename T, typename... Args>
  void emplaceComponent(Args&&... args)
  {
    ASSERT_SUBCLASS_OF_COMPONENT(T);
    components[T::componentId()].push_back(std::make_unique<T>(args...));
  }

  /**
   * Insert a new component of type T referenced by a owning pointer. This
   * operation moves the pointer into the Entity.
   */
  template <typename T>
  void insertComponent(std::unique_ptr<T>&& comp_ptr)
  {
    ASSERT_SUBCLASS_OF_COMPONENT(T);
    components[T::componentId()].push_back(std::move(comp_ptr));
  }

  /**
   * Destroy the component of type T stored at the given pointer location.
   */
  template <typename T>
  void removeComponent(const components::BaseComponent* comp_ptr)
  {
    ASSERT_SUBCLASS_OF_COMPONENT(T);
    auto it = components.find(T::componentId());
    removeWithIterByPtr(it, comp_ptr);
  }

 private:
  std::string name;
  std::unique_ptr<Transform> transform;
  ComponentMap components;

  void removeWithIterByPtr(ComponentMap::iterator it,
                           const components::BaseComponent* ptr);
};

};  // namespace seng::scene

namespace std {

// Template instantiation for hash<>, so that we can use entities in sets and maps
template <>
struct hash<seng::scene::Entity> {
  std::size_t operator()(const seng::scene::Entity& entity) const
  {
    hash<std::string> stringHasher;
    return stringHasher(entity.getName());
  }
};

};  // namespace std
