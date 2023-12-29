#pragma once

#include <seng/input_enums.hpp>

#include <vector>

namespace seng {

// Forward declarations
namespace rendering {
class GlfwWindow;
}  // namespace rendering

/**
 * Simple class that monitors keyboard output for events.
 *
 * It uses 2 buffers to "compress" events in a frame-to-frame way:
 *
 * 1. `staging` buffer receives all changes that happened during this fram from
 *    the window
 * 2. `stored` buffer stores the previous's frame events
 *
 * At the end of each frame the application loop should call `updateEvents` to
 * "commit" the staging buffer and bring new events into the staging buffer.
 *
 * It is only movable, not copyable.
 */
class InputManager {
  /**
   * Needed friend since the main render loop needs to tell us when to commit
   * the staging buffer
   */
  friend class Application;

 public:
  /// Construct a new InputManager
  InputManager(rendering::GlfwWindow &window);
  InputManager(InputManager &&) = delete;
  InputManager(const InputManager &) = delete;

  InputManager &operator=(InputManager &&) = delete;
  InputManager &operator=(const InputManager &) = delete;

  /**
   * Returns true if during this frame the user has begun pressing the key
   * indicated with the given KeyCode.
   */
  bool keyDown(KeyCode key) const;

  /**
   * Returns true if during this frame the user has released the key
   * indicated with the given KeyCode.
   */
  bool keyUp(KeyCode key) const;

  /**
   * Returns true for all frames between a key's pressing and release.
   */
  bool keyHold(KeyCode key) const;

 private:
  const rendering::GlfwWindow *m_window;
  bool m_dirty;  // used for avoiding useless copying
  std::vector<bool> m_staging, m_stored;

  /**
   * Commit the staging buffer and pool new events.
   */
  void updateEvents();
};

}  // namespace seng
