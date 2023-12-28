#pragma once

#include <seng/components/base_component.hpp>
#include <seng/components/definitions.hpp>

#include <memory>
#include <string>
#include <unordered_map>

// Fwd decl
namespace YAML {
class Node;
};

// Fwd decl
namespace seng {
class Application;
}  // namespace seng

namespace seng::scene {
class Entity;
};  // namespace seng::scene

namespace seng::components {

/**
 * Static factory for creating Components from the scene config file.
 *
 * Components are registered statically by inheriting the ConfigParsableComponent<T>
 * templated mixin class. See the documentation for that for more info.
 *
 */
class SceneConfigComponentFactory {
 public:
  /// Function type to be implemented by parseable components.
  using TConfigCreateFunc = std::unique_ptr<BaseComponent> (*)(Application &,
                                                               scene::Entity &,
                                                               const YAML::Node &);

 public:
  /// Deleted constructor. Class is fully static.
  SceneConfigComponentFactory() = delete;

  static bool registerComponent(const ComponentIdType &name, TConfigCreateFunc create);

  /**
   * Create an instance of the Component identified by `name` from the given YAML
   * node.
   */
  static std::unique_ptr<BaseComponent> create(Application &app,
                                               scene::Entity &entity,
                                               const std::string &name,
                                               const YAML::Node &configNode);

 private:
  /// Convenience alias
  using FuncStore = std::unordered_map<ComponentIdType, TConfigCreateFunc>;

  /// Implements Construct on First Use idiom to avoid issues with static init
  static FuncStore &configCreateFuncs();
};

/**
 * Mixin class for statically registering a Component to SceneConfigComponentFactory.
 *
 * To register to the factory, a Component should inherit from this mixin and provide
 * two static methods:
 *
 * 1. `string componentId()`: return the id of the Component
 * 2. `unique_ptr<Component> createFromConfig(const Application &, const YAML::Node &)`:
 *    create a component instance from the YAML config
 */
template <typename T>
class ConfigParsableComponent {
 public:
  /**
   * Used only to force instantiation. Does nothing.
   */
  ConfigParsableComponent() { (void)&registered; }
  virtual ~ConfigParsableComponent() {}

 private:
  static bool registered;
};

template <typename T>
bool ConfigParsableComponent<T>::registered =
    SceneConfigComponentFactory::registerComponent(T::componentId(), T::createFromConfig);

};  // namespace seng::components
