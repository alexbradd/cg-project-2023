#include <memory>
#include <seng/application.hpp>
#include <seng/glfw_window.hpp>
#include <seng/log.hpp>
#include <seng/vulkan_renderer.hpp>

using namespace std;
using namespace seng;
using namespace seng::rendering;

Application::Application() : Application(ApplicationConfig{}) {}
Application::Application(ApplicationConfig& config)
    : Application(std::move(config)) {}
Application::Application(ApplicationConfig&& config) : conf{config} {}
Application::~Application() { destroyWindow(); }

void Application::run(unsigned int width, unsigned int height) {
  makeWindow(width, height);
  while (!window->shouldClose()) {
    window->poll();
    vulkan->draw();
  }
  destroyWindow();
}

void Application::makeWindow(unsigned int width, unsigned int height) {
  window = make_shared<GlfwWindow>(conf.appName, width, height);
  vulkan = make_unique<VulkanRenderer>(conf, *window);
  window->onResize([this](GLFWwindow*, unsigned int, unsigned int) {
    if (vulkan != nullptr) vulkan->signalResize();
  });
}

void Application::destroyWindow() {
  window = nullptr;
  vulkan = nullptr;
}

const ApplicationConfig& Application::config() const { return conf; }
