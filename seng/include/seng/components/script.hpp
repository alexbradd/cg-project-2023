#pragma once

#include <seng/components/definitions.hpp>
#include <seng/components/toggle.hpp>
#include <seng/scene/scene.hpp>

namespace seng {
class Entity;

/**
 * Base class for user-definable arbitrary scripts
 *
 * This Component provides convenience hooks into the Scene events via virtual
 * functions that the user can override. More details can be found in the
 * documentation of each of those.
 */
class ScriptComponent : public ToggleComponent {
 public:
  /// Constructor
  ScriptComponent(Entity &entity, bool enabled = true);
  ScriptComponent(const ScriptComponent &) = delete;
  ScriptComponent(ScriptComponent &&) = delete;
  ~ScriptComponent();

  ScriptComponent &operator=(const ScriptComponent &) = delete;
  ScriptComponent &operator=(ScriptComponent &&) = delete;

  DECLARE_COMPONENT_ID("ScriptComponent");

  /**
   * Override of BaseComponent's `initialize()`. It has been marked final since
   * it is used for internal setup and users are expected to extend
   * `scriptInitialize()` instead, which serves an identical purpose.
   */
  virtual void initialize() override final;

  /**
   * Component initialization routine.
   */
  virtual void scriptInitialize() {}

  /**
   * Run on the EARLY_UPDATE event if the component is active.
   *
   * In this stage, we are at the earliest point in time: the frame has just been
   * started.
   */
  virtual void onEarlyUpdate([[maybe_unused]] float deltaTime) {}

  /**
   * Run on the UPDATE event if the component is active.
   *
   * In this stage, we are at the center of the update loop: every system is
   * fully operational and we making all changes just before they are drawn to
   * screen.
   */
  virtual void onUpdate([[maybe_unused]] float deltaTime) {}

  /**
   * Run on the LATE_UPDATE event if the component is active.
   *
   * In this stage, we are at the end of the update loop the frame has been drawn,
   * but recording is not yet finished; final cleanups are being done.
   */
  virtual void onLateUpdate([[maybe_unused]] float deltaTime) {}

 private:
  SceneEventToken m_earlyUpdateToken, m_updateToken, m_lateUpdateToken;

  void onEarlyUpdateImpl(float deltaTime);
  void onUpdateImpl(float deltaTime);
  void onLateUpdateImpl(float deltaTime);
};

};  // namespace seng
