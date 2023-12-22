#pragma once

#include <seng/components/definitions.hpp>

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
 * The BaseComponent class provides a constructor that initializes the
 * `application` and `entity` protected fields, containing respecitvely the
 * current Application instance and the Entity to which the component is
 * attached. Inheritors may use this constructor or initialize these fields
 * themselves. Well-behaved constructors should not leave these fields nullptr.
 */
class BaseComponent {
 public:
  BaseComponent() = default;
  virtual ~BaseComponent() = default;

  DECLARE_COMPONENT_ID("__BaseComponent");
};

};  // namespace seng::components
