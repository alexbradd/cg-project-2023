#pragma once

// clang-format off
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
// clang-format on

#include <functional>
#include <optional>
#include <seng/log.hpp>
#include <string>

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
  GlfwWindow(GlfwWindow &&) noexcept;
  ~GlfwWindow();

  GlfwWindow &operator=(const GlfwWindow &) = delete;
  GlfwWindow &operator=(GlfwWindow &&);

  GLFWwindow *getPointer() const {
    seng::log::warning("FIXME: remove access to internal pointer");
    return ptr;
  }

  /**
   * Return true if the user has request window closure.
   */
  bool shouldClose() const;

  /**
   * Run the given callback on window resizing.
   *
   * The parameters passed to the callback are the same passed
   * glfwSetFramebufferSizeCallback().
   */
  void onResize(std::function<void(GLFWwindow *, int, int)> callback);

  // getters for various properties
  const std::string &appName() const;
  unsigned int width() const;
  unsigned int height() const;
  std::vector<const char *> extensions() const;
  std::pair<unsigned int, unsigned int> framebufferSize() const;

  /**
   * Equivalent to calling glfwWaitEvents().
   */
  void wait() const;

  /**
   * Create the underlying Vulkan surface.
   */
  vk::raii::SurfaceKHR createVulkanSurface(vk::raii::Instance &) const;

 private:
  GLFWwindow *ptr;
  std::string _appName;
  unsigned int _width, _height;
  std::optional<std::function<void(GLFWwindow *, int, int)>> _onResize;

  static void resizeCallback(GLFWwindow *window, int w, int h);
};

}  // namespace seng::rendering
