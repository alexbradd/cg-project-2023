#include <seng/glfw_window.hpp>
#include <seng/input_enums.hpp>
#include <seng/input_manager.hpp>
#include <seng/log.hpp>

using namespace seng;

// Quick macro since I am lazy and don't want to type static_cast everywhere
#define toInt(a) static_cast<int>(a)

// Helpers to fit all key codes into a single [0;KEY_RANGE) range
constexpr int KEY_RANGE = toInt(KeyCode::eModMenu) - toInt(KeyCode::eSpace);
static int intoRange(int code) { return code - toInt(KeyCode::eSpace); }
static int intoRange(KeyCode code) { return intoRange(toInt(code)); }

InputManager::InputManager(rendering::GlfwWindow& window) :
    window(window),
    dirty(false),
    staging(KEY_RANGE, false),
    stored(KEY_RANGE, false) {
  this->window.get().onKeyEvent(
      [&](GLFWwindow*, int key, int, int action, int) {
        if (key == -1) return;
        if (action == toInt(KeyEvent::ePress)) {
          dirty = true;
          staging[intoRange(key)] = true;
        } else if (action == toInt(KeyEvent::eRelease)) {
          dirty = true;
          staging[intoRange(key)] = false;
        }
      });
}

void InputManager::updateEvents() {
  if (dirty) {
    stored.assign(staging.begin(), staging.end());
    dirty = false;
  }
  window.get().poll();
}

bool InputManager::keyDown(KeyCode code) {
  if (staging[intoRange(code)] == true && stored[intoRange(code)] == false)
    return true;
  return false;
}

bool InputManager::keyHold(KeyCode code) {
  if (staging[intoRange(code)] == true && stored[intoRange(code)] == true)
    return true;
  return false;
}

bool InputManager::keyUp(seng::KeyCode code) {
  if (staging[intoRange(code)] == false && stored[intoRange(code)] == true)
    return true;
  return false;
}
