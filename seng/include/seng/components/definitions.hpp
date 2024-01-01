#pragma once

#include <string>

/// Convenience macro to declare a Component's ID
#define DECLARE_COMPONENT_ID(id)               \
  static ::seng::ComponentIdType componentId() \
  {                                            \
    return id;                                 \
  }

namespace seng {

/// Convenience type alias for the type used by the Component's ID
using ComponentIdType = std::string;

};  // namespace seng
