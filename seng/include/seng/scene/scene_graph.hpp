#pragma once

#include <seng/scene/entity.hpp>

#include <list>

namespace seng::scene {

/**
 * The scene graph.
 *
 * A scene graph is an ADG of entities. An arc represents a parent-child relationship.
 * Thus a hierachy of Entities is created.
 *
 * FIXME: this is a simple non-hierarchical version. If I have time I will implement
 * the hierarchicality??.
 */
class SceneGraph {
 public:
  /// Convenience typedef
  using EntityList = std::list<Entity>;

 public:
  /// Constructor
  SceneGraph() = default;
  SceneGraph(const SceneGraph &) = delete;
  SceneGraph(SceneGraph &&) = default;

  SceneGraph &operator=(const SceneGraph &) = delete;
  SceneGraph &operator=(SceneGraph &&) = default;

  ~SceneGraph() = default;

  /**
   * Find an Entity in the scene graph by its name. If no such entity can be found,
   * a past-the-end iterator is returned.
   *
   * N.B.: searching for an entity is linear in the number of entites in the
   * scene graph, so make sure to cache the iterator (it is safe to do
   * so since they are guaranteed to not be invaliedated w.r.t. scene graph changes).
   */
  EntityList::const_iterator findByName(const std::string &name) const;

  /**
   * Find an Entity in the scene graph by its name. If no such entity can be found,
   * a past-the-end iterator is returned.
   *
   * N.B.: searching for an entity is linear in the number of entites in the
   * scene graph, so make sure to cache the iterator (it is safe to do
   * so since they are guaranteed to not be invaliedated w.r.t. scene graph changes).
   */
  EntityList::iterator findByName(const std::string &name);

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
  void remove(const Entity *e);

  /**
   * Delete the Entity with the given name from the scene graph.
   *
   * N.B. This incurs linear search time.
   */
  void remove(const std::string &name);

  /**
   * Delete the Entity referenced by the given iterator
   */
  void remove(EntityList::const_iterator it);

  /**
   * Clear the scene graph
   */
  void clear();

 private:
  EntityList entities;
};

}  // namespace seng::scene
