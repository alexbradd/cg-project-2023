#pragma once

#include <seng/components/base_component.hpp>
#include <seng/components/definitions.hpp>

namespace seng {
class Entity;

/**
 * Add support for enabling/disabling.
 */
class ToggleComponent : public BaseComponent {
 public:
  /// Constructor
  ToggleComponent(Entity &entity, bool enabled = true);
  ToggleComponent(const ToggleComponent &) = delete;
  ToggleComponent(ToggleComponent &&) = delete;

  ToggleComponent &operator=(const ToggleComponent &) = delete;
  ToggleComponent &operator=(ToggleComponent &&) = delete;

  DECLARE_COMPONENT_ID("ToggleComponent");

  // Accessors
  bool enabled() const { return m_enabled; }
  void enabled(bool b);

  /**
   * Enable the Component. `onEnable` is called.
   */
  void enable();

  /**
   * Disable the Component. `onDisable` is called.
   */
  void disable();

  /**
   * Toggle the Component. `onDisable` or `onEnable` are called depending on
   * the state.
   */
  void toggle();

  /**
   * Hook to execute code when the Component is enabled.
   */
  virtual void onEnable() {}

  /**
   * Hook to execute code when the Component is disabled.
   */
  virtual void onDisable() {}

 private:
  bool m_enabled;
};

}  // namespace seng
