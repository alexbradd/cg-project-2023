#pragma once

#include <string>

/// Convenience macro to declare a Component's ID
#define DECLARE_COMPONENT_ID(id)                           \
  static ::seng::components::ComponentIdType componentId() \
  {                                                        \
    return id;                                             \
  }

namespace seng::components {

/// Convenience type alias for the type used by the Component's ID
using ComponentIdType = std::string;

};  // namespace seng::components
