#pragma once

#include <seng/components/definitions.hpp>

#include <memory>

namespace seng {
class Application;
}  // namespace seng

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
 */
class BaseComponent {
 public:
  DECLARE_COMPONENT_ID("__BaseComponent");

  /**
   * Constructor.
   *
   * Overloaded constructors should not do anything more than set the needed
   * parameters. Any interactions with the other engin systems should be done
   * in `initialize()`
   */
  BaseComponent(scene::Entity &entity) : entity(std::addressof(entity)) {}
  virtual ~BaseComponent() = default;

  /**
   * Initialize this component.
   *
   * All systems and reference are guaranteed to be correct when this function is
   * called, maning it is safe to do anything inside.
   */
  virtual void initialize() {}

 protected:
  scene::Entity *entity;
};

};  // namespace seng::components
