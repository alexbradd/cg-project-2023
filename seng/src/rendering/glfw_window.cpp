// clang-format off
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
// clang-format on

#include <seng/rendering/glfw_window.hpp>

#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace seng;
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

  // Callbacks
  glfwSetFramebufferSizeCallback(ptr, resizeCallback);
  glfwSetKeyCallback(ptr, onKeyCallback);
}

GlfwWindow::~GlfwWindow() {
  glfwDestroyWindow(ptr);
  glfwTerminate();
}

void GlfwWindow::close() { glfwSetWindowShouldClose(ptr, GLFW_TRUE); }

bool GlfwWindow::shouldClose() const { return glfwWindowShouldClose(ptr); }

void GlfwWindow::onResize(function<void(GLFWwindow *, int, int)> callback) {
  _onResizeCbs.push_back(callback);
}

void GlfwWindow::onKeyEvent(
    function<void(GLFWwindow *, int, int, int, int)> callback) {
  _onKeyEvent = callback;
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
  for (const auto& onResize: wrapper->_onResizeCbs)
    onResize(window, w, h);
}

void GlfwWindow::onKeyCallback(
    GLFWwindow *window, int key, int scancode, int action, int mods) {
  auto wrapper =
      reinterpret_cast<GlfwWindow *>(glfwGetWindowUserPointer(window));
  if (wrapper->_onKeyEvent.has_value())
    (wrapper->_onKeyEvent).value()(window, key, scancode, action, mods);
}
