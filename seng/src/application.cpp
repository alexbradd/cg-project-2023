#include <memory>
#include <seng/application.hpp>
#include <seng/glfw_window.hpp>
#include <seng/log.hpp>

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
  window = shared_ptr<GlfwWindow>(new GlfwWindow{appName, width, height});
}

void Application::destroyWindow() {
  window = nullptr;
}

const string& Application::getShaderPath() { return shaderPath; }
void Application::setShaderPath(string s) { shaderPath = s; }

const string& Application::getModelPath() { return modelPath; }
void Application::setModelPath(string s) { modelPath = s; }

const string& Application::getAppName() { return appName; }
