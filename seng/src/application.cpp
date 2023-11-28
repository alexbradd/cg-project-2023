#include <memory>
#include <seng/application.hpp>
#include <seng/glfwWindowWrapper.hpp>
#include <seng/log.hpp>
#include <seng/vulkan_internals.hpp>

using namespace std;
using namespace seng;
using namespace seng::internal;

Application::Application() : Application("Vulkan") {}
Application::Application(string appName) : appName{appName} {}
Application::~Application() {}

void Application::run(unsigned int width, unsigned int height) {
  makeWindow(width, height);
  while (!window->shouldClose()) {
    glfwPollEvents();
    vulkan->drawFrame();
  }
  destroyWindow();
}

void Application::makeWindow(unsigned int width, unsigned int height) {
  window = shared_ptr<GlfwWindowWrapper>(
      new GlfwWindowWrapper{appName, width, height});
  vulkan = shared_ptr<VulkanInternals>(new VulkanInternals{*this});
  window->setonResize([this](GLFWwindow* ptr, unsigned int w, unsigned int h) {
    vulkan->signalResize();
  });
}

void Application::destroyWindow() {
  window = nullptr;
  vulkan = nullptr;
}

shared_ptr<GlfwWindowWrapper> Application::getWindow() { return window; }

const string& Application::getShaderPath() { return shaderPath; }
void Application::setShaderPath(string s) { shaderPath = s; }

const string& Application::getModelPath() { return modelPath; }
void Application::setModelPath(string s) { modelPath = s; }

const string& Application::getAppName() { return appName; }
