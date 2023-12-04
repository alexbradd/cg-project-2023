// clang-format off
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
// clang-format on

#include <cstdint>
#include <seng/glfw_window.hpp>
#include <utility>

using namespace std;
using namespace seng::rendering;

GlfwWindow::GlfwWindow(string appName,
                       unsigned int width,
                       unsigned int height) :
    _appName(std::move(appName)), _width(width), _height(height) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  ptr = glfwCreateWindow(width, height, _appName.c_str(), nullptr, nullptr);
  glfwSetWindowUserPointer(ptr, this);
  glfwSetFramebufferSizeCallback(ptr, resizeCallback);
}

GlfwWindow::~GlfwWindow() {
  glfwDestroyWindow(ptr);
  glfwTerminate();
}

bool GlfwWindow::shouldClose() const { return glfwWindowShouldClose(ptr); }

void GlfwWindow::onResize(function<void(GLFWwindow *, int, int)> callback) {
  _onResize = callback;
}

vector<const char *> GlfwWindow::extensions() const {
  uint32_t ext_count;
  vector<const char *> vec{};
  const char **glfw_ext = glfwGetRequiredInstanceExtensions(&ext_count);
  vec.resize(ext_count);
  for (uint32_t i = 0; i < ext_count; i++) vec[i] = glfw_ext[i];
  return vec;
}

pair<unsigned int, unsigned int> GlfwWindow::framebufferSize() const {
  int w, h;
  glfwGetFramebufferSize(ptr, &w, &h);
  return std::pair(w, h);
}

void GlfwWindow::wait() const { glfwWaitEvents(); }

void GlfwWindow::poll() const { glfwPollEvents(); }

vk::raii::SurfaceKHR GlfwWindow::createVulkanSurface(
    vk::raii::Instance &i) const {
  VkSurfaceKHR surf;
  auto res = glfwCreateWindowSurface(*i, ptr, nullptr, &surf);
  if (res != VK_SUCCESS)
    throw std::runtime_error("failed to create window surface!");
  return vk::raii::SurfaceKHR{i, surf};
}

void GlfwWindow::resizeCallback(GLFWwindow *window, int w, int h) {
  auto wrapper =
      reinterpret_cast<GlfwWindow *>(glfwGetWindowUserPointer(window));
  wrapper->_width = w;
  wrapper->_height = h;
  if (w == 0 || h == 0) return;
  if (wrapper->_onResize.has_value())
    (wrapper->_onResize).value()(window, w, h);
}
