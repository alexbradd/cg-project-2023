#ifndef __PRIVATE_SENG_GLFW_WINDOW_WRAPPER_HPP__
#define __PRIVATE_SENG_GLFW_WINDOW_WRAPPER_HPP__

// clang-format off
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
// clang-format on

#include <string>

namespace seng::internal {

class GlfwWindowWrapper {
 public:
  GlfwWindowWrapper(const std::string &appName, unsigned int width,
                    unsigned int height) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,
                   GLFW_FALSE);  // FIXME: make it resizeable when ready
    ptr = glfwCreateWindow(width, height, appName.c_str(), nullptr, nullptr);
  }
  ~GlfwWindowWrapper() {
    glfwDestroyWindow(ptr);
    glfwTerminate();
  }

  GLFWwindow *getPointer() { return ptr; }
  bool shouldClose() { return glfwWindowShouldClose(ptr); }

 private:
  GLFWwindow *ptr;
};
}  // namespace seng::internal

#endif  // __PRIVATE_SENG_GLFW_WINDOW_WRAPPER_HPP__
