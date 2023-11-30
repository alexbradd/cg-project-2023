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
 * Wrapper arond the C API provided by glfw3.
 *
 * This class represents a GLFW window. Since we should have only 1 window, the
 * class calls glfwInit() at creation and glfwTerminate at deallocation.
 *
 * It non copyable but movable.
 */
class GlfwWindow {
 public:
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
  bool shouldClose() const;
  void onResize(std::function<void(GLFWwindow *, int, int)> callback);
  const std::string &appName() const;
  unsigned int width() const;
  unsigned int height() const;
  std::vector<const char *> extensions() const;
  std::pair<unsigned int, unsigned int> framebufferSize() const;
  void wait() const;
  vk::raii::SurfaceKHR createVulkanSurface(vk::raii::Instance &) const;

 private:
  GLFWwindow *ptr;
  std::string _appName;
  unsigned int _width, _height;
  std::optional<std::function<void(GLFWwindow *, int, int)>> _onResize;

  static void resizeCallback(GLFWwindow *window, int w, int h);
};

}  // namespace seng::rendering
