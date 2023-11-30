#include <cstdint>
#include <seng/glfw_window.hpp>
#include <utility>
#include <vulkan/vulkan_raii.hpp>

using namespace std;
using namespace seng::internal;

GlfwWindow::GlfwWindow(string appName, unsigned int width, unsigned int height)
    : _appName(std::move(appName)), _width(width), _height(height) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  ptr = glfwCreateWindow(width, height, _appName.c_str(), nullptr, nullptr);
  glfwSetWindowUserPointer(ptr, this);
  glfwSetFramebufferSizeCallback(ptr, resizeCallback);
}

GlfwWindow::GlfwWindow(GlfwWindow &&other) noexcept
    : ptr(exchange(other.ptr, nullptr)),
      _appName(std::move(other._appName)),
      _width(exchange(other._width, 0)),
      _height(exchange(other._height, 0)),
      _onResize(std::move(other._onResize)) {}

GlfwWindow::~GlfwWindow() {
  glfwDestroyWindow(ptr);
  glfwTerminate();
}

GlfwWindow &GlfwWindow::operator=(GlfwWindow &&other) {
  if (this != &other) {
    if (this->ptr != nullptr) glfwDestroyWindow(this->ptr);
    this->ptr = exchange(other.ptr, nullptr);
    this->_appName = other._appName;
    this->_width = exchange(other._width, 0);
    this->_height = exchange(other._height, 0);
    this->_onResize = std::move(other._onResize);
    glfwSetWindowUserPointer(this->ptr, this);
  }
  return *this;
}

bool GlfwWindow::shouldClose() const { return glfwWindowShouldClose(ptr); }

void GlfwWindow::onResize(function<void(GLFWwindow *, int, int)> callback) {
  _onResize = callback;
}

const string &GlfwWindow::appName() const { return _appName; }

unsigned int GlfwWindow::width() const { return _width; }

unsigned int GlfwWindow::height() const { return _height; }

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
