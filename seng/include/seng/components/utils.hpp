#pragma once

#include <memory>
#include <type_traits>

/// Assert that type `typ` is a subclass of BaseComponent
#define ASSERT_SUBCLASS_OF_COMPONENT(typ)                                      \
  do {                                                                         \
    static_assert(std::is_base_of<seng::BaseComponent, typ>::value,            \
                  "Type must be subclass of seng::components::BaseComponent"); \
  } while (0)

namespace seng {

class BaseComponent;

/// Convert a generic unique_ptr to the concrete version
template <typename Concrete>
std::unique_ptr<Concrete> concreteUniquePtr(std::unique_ptr<BaseComponent>&& ptr)
{
  ASSERT_SUBCLASS_OF_COMPONENT(Concrete);
  BaseComponent* raw = ptr.release();
  return std::unique_ptr<Concrete>(static_cast<Concrete*>(raw));
}

/// Convert a generic pointer to the concrete version
template <typename Concrete>
const Concrete* concretePtr(const BaseComponent* ptr)
{
  ASSERT_SUBCLASS_OF_COMPONENT(Concrete);
  return static_cast<const Concrete*>(ptr);
}

/// Convert a generic pointer to the concrete version
template <typename Concrete>
Concrete* concretePtr(BaseComponent* ptr)
{
  ASSERT_SUBCLASS_OF_COMPONENT(Concrete);
  return static_cast<Concrete*>(ptr);
}

}  // namespace seng
