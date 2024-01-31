#pragma once

#include <seng/components/base_component.hpp>
#include <seng/components/component_ptr.hpp>
#include <seng/components/definitions.hpp>

#include <memory>
#include <string>
#include <unordered_map>

/// Use for in-line definition of the YAML parsing function
#define DECLARE_CREATE_FROM_CONFIG()                                           \
  static std::unique_ptr<seng::BaseComponent> createFromConfig(seng::Entity &, \
                                                               const YAML::Node &)

/// Use for out-of-line definition of the YAML parsing function
#define DEFINE_CREATE_FROM_CONFIG(type, entity, node)                               \
  std::unique_ptr<seng::BaseComponent> type::createFromConfig(seng::Entity &entity, \
                                                              const YAML::Node &node)

/// Register the given type to SceneConfigComponentFactory
#define REGISTER_TO_CONFIG_FACTORY(type) \
  template class seng::ConfigParsableComponent<type>

// Fwd decl
namespace YAML {
class Node;
};

namespace seng {
class Entity;

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
  using TConfigCreateFunc = std::unique_ptr<BaseComponent> (*)(Entity &,
                                                               const YAML::Node &);

 public:
  /// Deleted constructor. Class is fully static.
  SceneConfigComponentFactory() = delete;

  static bool registerComponent(const ComponentIdType &name, TConfigCreateFunc create);

  /**
   * Create an instance of the Component identified by `name` from the given YAML
   * node.
   */
  static ComponentPtr create(Entity &entity,
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
 * To register to the factory, a Component should first inherit from this mixin
 * and provide two static methods:
 *
 * 1. `ComponentIdType componentId()`: return the id of the Component
 * 2. `unique_ptr<BaseComponent> createFromConfig(Entity &, const YAML::Node &)`:
 *    create a component instance from the YAML config
 *
 * Then it should instantiate this template.
 *
 * Convenience macros are provided for every step:
 * 1. DECLARE_COMPONENT_ID for defining componentId()
 * 2. DECLARE/DEFINE_CREATE_FROM_CONFIG for declarein/defining createFromConfig
 * 3. REGISTER_TO_CONFIG_FACTORY for instantiating the template
 */
template <typename T>
class ConfigParsableComponent {
 public:
  ConfigParsableComponent() {}
  virtual ~ConfigParsableComponent() {}

 private:
  static bool registered;
};

template <typename T>
bool ConfigParsableComponent<T>::registered =
    SceneConfigComponentFactory::registerComponent(T::componentId(), T::createFromConfig);

};  // namespace seng
