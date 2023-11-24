#include <memory>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <seng/application.hpp>
#include <seng/glfwWindowWrapper.hpp>
#include <seng/log.hpp>
#include <seng/vulkan_internals.hpp>

using namespace std;
using namespace seng;
using namespace seng::internal;

class Application::Context {
 public:
  Context(string &appName, unsigned int w, unsigned int h)
      : window{appName.c_str(), w, h},
        vulkan{window.ptr, appName, w, h} {}

  void mainLoop() {
    while (!window.shouldClose()) {
      glfwPollEvents();
    }
  }

 private:
  GlfwWindowWrapper window;
  VulkanInternals vulkan;
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
