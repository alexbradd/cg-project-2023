#ifndef __PRIVATE_SENG_GLFW_WINDOW_WRAPPER_HPP__
#define __PRIVATE_SENG_GLFW_WINDOW_WRAPPER_HPP__

// clang-format off
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
// clang-format on

#include <functional>
#include <optional>
#include <string>

namespace seng::internal {

class GlfwWindowWrapper {
 public:
  GlfwWindowWrapper(const std::string &appName, unsigned int width,
                    unsigned int height)
      : width{width}, height{height} {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    ptr = glfwCreateWindow(width, height, appName.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(ptr, this);
    glfwSetFramebufferSizeCallback(ptr, resizeCallback);
  }
  ~GlfwWindowWrapper() {
    glfwDestroyWindow(ptr);
    glfwTerminate();
  }

  GLFWwindow *getPointer() { return ptr; }
  bool shouldClose() { return glfwWindowShouldClose(ptr); }
  void setonResize(std::function<void(GLFWwindow *, int, int)> callback) {
    onResize = callback;
  }
  unsigned int getWidth() { return width; }
  unsigned int getHeight() { return height; }
  std::pair<unsigned int, unsigned int> getFramebufferSize() {
    int w, h;
    glfwGetFramebufferSize(ptr, &w, &h);
    return std::pair(w, h);
  }
  void wait() { glfwWaitEvents(); }

 private:
  GLFWwindow *ptr;
  unsigned int width, height;
  std::optional<std::function<void(GLFWwindow *, int, int)>> onResize;

  static void resizeCallback(GLFWwindow *window, int w, int h) {
    auto wrapper =
        reinterpret_cast<GlfwWindowWrapper *>(glfwGetWindowUserPointer(window));
    wrapper->width = w;
    wrapper->height = h;
    if (w == 0 || h == 0) return;
    if (wrapper->onResize.has_value())
      (wrapper->onResize).value()(window, w, h);
  }
};
}  // namespace seng::internal

#endif  // __PRIVATE_SENG_GLFW_WINDOW_WRAPPER_HPP__
