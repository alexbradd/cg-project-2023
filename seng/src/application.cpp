#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <seng/application.hpp>
#include <seng/log.hpp>
#include <seng/vulkan_internals.hpp>

using namespace std;
using namespace seng;
using namespace seng::internal;

struct GlfwWindowWrapper {
  GLFWwindow *ptr;

  GlfwWindowWrapper(string appName, unsigned int width, unsigned int height) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,
                   GLFW_FALSE);  // FIXME: make it resizeable when ready
    ptr = glfwCreateWindow(width, height, appName.c_str(), nullptr, nullptr);
  }

  bool shouldClose() { return glfwWindowShouldClose(ptr); }

  ~GlfwWindowWrapper() {
    glfwDestroyWindow(ptr);
    glfwTerminate();
  }
};

class Application::impl {
 public:
  string appName;
  unsigned int width, height;

  GlfwWindowWrapper window;
  VulkanInternals vulkan;

  impl(string appName, unsigned int w, unsigned int h)
      : appName{appName},
        width{w},
        height{h},
        window{GlfwWindowWrapper(appName.c_str(), w, h)},
        vulkan{VulkanInternals(window.ptr, appName, w, h)} {}

  void initWindow() {}

  void mainLoop() {
    while (!window.shouldClose()) {
      glfwPollEvents();
    }
  }
};

Application::Application() : Application("Vulkan", 800, 600) {}
Application::Application(string window_name, unsigned int width,
                         unsigned int height)
    : pimpl{new impl{window_name, width, height}} {}
Application::~Application() {}

void Application::run() { pimpl->mainLoop(); }
