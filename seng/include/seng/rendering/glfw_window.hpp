#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

struct GLFWwindow;
namespace vk::raii {
class Instance;
class SurfaceKHR;
}  // namespace vk::raii

namespace seng::rendering {

/**
 * Wrapper arond the C API provided by glfw3, it represents a GLFW window.
 *
 * It implements the RAII pattern, meaning that instantiating the class
 * allocates, all resources, while destruction of the class deallocate them.
 * Since we should have only 1 window, the class calls glfwInit() at instation
 * and glfwTerminate() at destruction.
 *
 * It non copyable but movable.
 */
class GlfwWindow {
 public:
  /**
   * Create a new GlfwWindow with name `appname` and the give starting width and
   * height.
   *
   * Creating a new GlfwWindow automatically show the window in question.
   */
  GlfwWindow(std::string appName, unsigned int width, unsigned int height);
  GlfwWindow(const GlfwWindow &) = delete;
  GlfwWindow(GlfwWindow &&) = default;
  ~GlfwWindow();

  GlfwWindow &operator=(const GlfwWindow &) = delete;
  GlfwWindow &operator=(GlfwWindow &&) = default;

  /**
   * Return true if the user has request window closure.
   */
  bool shouldClose() const;

  /**
   * Request window closure.
   */
  void close();

  /**
   * Add the given callback to the list of functions to be run on window resizing.
   *
   * The parameters passed to the callback are the same passed
   * glfwSetFramebufferSizeCallback().
   */
  void onResize(std::function<void(GLFWwindow *, int, int)> callback);

  /**
   * Run the given callback on key events.
   *
   * The parameters passed to the callback are the same passed
   * glfwSetKeyCallback().
   */
  void onKeyEvent(std::function<void(GLFWwindow *, int, int, int, int)> cb);

  // getters for various properties
  const std::string &appName() const { return m_appName; }
  unsigned int width() const { return m_width; }
  unsigned int height() const { return m_height; }

  std::vector<const char *> extensions() const;
  std::pair<unsigned int, unsigned int> framebufferSize() const;

  /**
   * Equivalent to calling glfwWaitEvents().
   */
  void wait() const;

  /**
   * Equivalent to calling glfwPollEvents().
   */
  void poll() const;

  /**
   * Create the underlying Vulkan surface.
   */
  vk::raii::SurfaceKHR createVulkanSurface(vk::raii::Instance &) const;

 private:
  GLFWwindow *m_ptr;
  std::string m_appName;
  unsigned int m_width, m_height;
  std::vector<std::function<void(GLFWwindow *, int, int)>> m_resizeCbs;
  std::optional<std::function<void(GLFWwindow *, int, int, int, int)>> m_keyEventCb;

  static void resizeCallback(GLFWwindow *window, int w, int h);
  static void onKeyCallback(
      GLFWwindow *window, int key, int scancode, int action, int mods);
};

}  // namespace seng::rendering
