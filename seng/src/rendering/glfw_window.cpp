// clang-format off
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
// clang-format on

#include <seng/rendering/glfw_window.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace seng;
using namespace seng::rendering;

GlfwWindow::GlfwWindow(string appName, unsigned int width, unsigned int height) :
    m_appName(std::move(appName)), m_width(width), m_height(height)
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  m_ptr = glfwCreateWindow(width, height, m_appName.c_str(), nullptr, nullptr);
  glfwSetWindowUserPointer(m_ptr, this);

  // Callbacks
  glfwSetFramebufferSizeCallback(m_ptr, resizeCallback);
  glfwSetKeyCallback(m_ptr, onKeyCallback);
}

GlfwWindow::~GlfwWindow()
{
  glfwDestroyWindow(m_ptr);
  glfwTerminate();
}

void GlfwWindow::close()
{
  glfwSetWindowShouldClose(m_ptr, GLFW_TRUE);
}

bool GlfwWindow::shouldClose() const
{
  return glfwWindowShouldClose(m_ptr);
}

vector<const char *> GlfwWindow::extensions() const
{
  uint32_t ext_count;
  vector<const char *> vec{};
  const char **glfw_ext = glfwGetRequiredInstanceExtensions(&ext_count);
  vec.resize(ext_count);
  for (uint32_t i = 0; i < ext_count; i++) vec[i] = glfw_ext[i];
  return vec;
}

pair<unsigned int, unsigned int> GlfwWindow::framebufferSize() const
{
  int w, h;
  glfwGetFramebufferSize(m_ptr, &w, &h);
  return std::pair(w, h);
}

void GlfwWindow::wait() const
{
  glfwWaitEvents();
}

void GlfwWindow::poll() const
{
  glfwPollEvents();
}

vk::raii::SurfaceKHR GlfwWindow::createVulkanSurface(vk::raii::Instance &i) const
{
  VkSurfaceKHR surf;
  auto res = glfwCreateWindowSurface(*i, m_ptr, nullptr, &surf);
  if (res != VK_SUCCESS) throw std::runtime_error("failed to create window surface!");
  return vk::raii::SurfaceKHR{i, surf};
}

void GlfwWindow::resizeCallback(GLFWwindow *window, int w, int h)
{
  auto wrapper = reinterpret_cast<GlfwWindow *>(glfwGetWindowUserPointer(window));
  wrapper->m_width = w;
  wrapper->m_height = h;
  if (w == 0 || h == 0) return;
  wrapper->m_resize(wrapper, w, h);
}

void GlfwWindow::onKeyCallback(
    GLFWwindow *window, int key, int scancode, int action, int mods)
{
  auto wrapper = reinterpret_cast<GlfwWindow *>(glfwGetWindowUserPointer(window));
  wrapper->m_keyEvent(wrapper, key, scancode, action, mods);
}
