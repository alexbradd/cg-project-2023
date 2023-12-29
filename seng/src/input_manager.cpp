#include <seng/input_manager.hpp>
#include <seng/rendering/glfw_window.hpp>

#include <functional>
#include <stdexcept>
#include <vector>

using namespace seng;

// Quick macro since I am lazy and don't want to type static_cast everywhere
#define toInt(a) static_cast<int>(a)

// Helpers to fit all key codes into a single [0;KEY_RANGE) range
constexpr int KEY_RANGE = toInt(KeyCode::eModMenu) - toInt(KeyCode::eSpace);
static int intoRange(int code)
{
  return code - toInt(KeyCode::eSpace);
}
static int intoRange(KeyCode code)
{
  return intoRange(toInt(code));
}

InputManager::InputManager(rendering::GlfwWindow* window) :
    // window is initialized later
    m_dirty(false), m_staging(KEY_RANGE, false), m_stored(KEY_RANGE, false)
{
  if (window == nullptr) throw std::runtime_error("Window should not be null");
  window->onKeyEvent([&](GLFWwindow*, int key, int, int action, int) {
    if (key == -1) return;
    if (action == toInt(KeyEvent::ePress)) {
      m_dirty = true;
      m_staging[intoRange(key)] = true;
    } else if (action == toInt(KeyEvent::eRelease)) {
      m_dirty = true;
      m_staging[intoRange(key)] = false;
    }
  });
  this->m_window = window;
}

void InputManager::updateEvents()
{
  if (m_dirty) {
    m_stored.assign(m_staging.begin(), m_staging.end());
    m_dirty = false;
  }
  m_window->poll();
}

bool InputManager::keyDown(KeyCode code) const
{
  if (m_staging[intoRange(code)] == true && m_stored[intoRange(code)] == false)
    return true;
  return false;
}

bool InputManager::keyHold(KeyCode code) const
{
  if (m_staging[intoRange(code)] == true && m_stored[intoRange(code)] == true)
    return true;
  return false;
}

bool InputManager::keyUp(seng::KeyCode code) const
{
  if (m_staging[intoRange(code)] == false && m_stored[intoRange(code)] == true)
    return true;
  return false;
}
