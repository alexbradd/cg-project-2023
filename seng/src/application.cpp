#include <memory>
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

class Application::Context {
 public:
  GlfwWindowWrapper window;
  VulkanInternals vulkan;

  Context(string &appName, unsigned int w, unsigned int h)
      : window{appName.c_str(), w, h},
        vulkan{window.ptr, appName, w, h} {}

  void mainLoop() {
    while (!window.shouldClose()) {
      glfwPollEvents();
    }
  }
};

Application::Application() : Application("Vulkan", 800, 600) {}
Application::Application(string appName, unsigned int width,
                         unsigned int height)
    : appName{appName},
      initialWidth{width},
      initialHeight{height},
      ctx{nullptr} {}
Application::~Application() {}

void Application::run() {
  ctx = unique_ptr<Context>(new Context(appName, initialWidth, initialHeight));
  ctx->mainLoop();
  ctx = unique_ptr<Context>(nullptr);
}
