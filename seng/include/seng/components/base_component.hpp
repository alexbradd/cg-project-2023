#pragma once

#include <seng/components/definitions.hpp>

#include <memory>

namespace seng::scene {
class Entity;
};  // namespace seng::scene

namespace seng::components {

/**
 * Root of the Component inheritance tree.
 *
 * To declare a new Component, one should extend this class, override the
 * needed virtual methods (if present) and declare the components ID, using
 * either:
 *
 * 1. The dedicated DECLARE_COMPONENT_ID(str) macro
 * 2. Declaring a static function called `componentId` that returns a
 *    ComponentIdType
 *
 * The BaseComponent class provides a constructor that initializes the `entity`
 * protected field, containing the Entity to which the component is attached.
 * Inheritors may use this constructor or initialize this field themselves.
 * Well-behaved constructors should not leave this field nullptr.
 *
 * Components should not be copyable or movable, since they are fixed in memory
 * and handled via smart pointers.
 */
class BaseComponent {
 public:
  DECLARE_COMPONENT_ID("__BaseComponent");

  /**
   * Constructor.
   *
   * Overloaded constructors should not do anything more than set the needed
   * parameters. Any interactions with the other engine systems should be done
   * in `initialize()` or other initialization functions.
   */
  BaseComponent(scene::Entity &entity) : entity(std::addressof(entity)) {}
  BaseComponent(const BaseComponent &) = delete;
  BaseComponent(BaseComponent &&) = delete;

  BaseComponent &operator=(const BaseComponent &) = delete;
  BaseComponent &operator=(BaseComponent &&) = delete;

  virtual ~BaseComponent() = default;

  /**
   * Initialize this component.
   *
   * All systems and reference are guaranteed to be correct when this function is
   * called, maning it is safe to do anything inside.
   */
  virtual void initialize() {}

 protected:
  scene::Entity *const entity;
};

};  // namespace seng::components
