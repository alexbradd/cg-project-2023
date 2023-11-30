#include <memory>
#include <seng/application.hpp>
#include <seng/glfw_window.hpp>
#include <seng/log.hpp>
#include <seng/vulkan_renderer.hpp>

using namespace std;
using namespace seng;
using namespace seng::rendering;

Application::Application() : Application("Vulkan") {}
Application::Application(string appName) : appName{appName} {}
Application::~Application() {}

void Application::run(unsigned int width, unsigned int height) {
  makeWindow(width, height);
  while (!window->shouldClose()) {
    glfwPollEvents();
  }
  destroyWindow();
}

void Application::makeWindow(unsigned int width, unsigned int height) {
  window = make_shared<GlfwWindow>(appName, width, height);
  vulkan = make_unique<VulkanRenderer>(*window);
  window->onResize([this](GLFWwindow*, unsigned int, unsigned int) {
    if (vulkan != nullptr) vulkan->signalResize();
  });
}

void Application::destroyWindow() {
  window = nullptr;
  vulkan = nullptr;
}

const string& Application::getShaderPath() { return shaderPath; }
void Application::setShaderPath(string s) { shaderPath = s; }

const string& Application::getModelPath() { return modelPath; }
void Application::setModelPath(string s) { modelPath = s; }

const string& Application::getAppName() { return appName; }
