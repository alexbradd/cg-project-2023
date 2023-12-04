#include <memory>
#include <seng/application.hpp>
#include <seng/glfw_window.hpp>
#include <seng/input_manager.hpp>
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

void Application::run(unsigned int width,
                      unsigned int height,
                      function<void(shared_ptr<InputManager>)> cb) {
  makeWindow(width, height);

  // The main applcation loop goes like this:
  //
  // 1. Game state is updated, reacting to input changes
  // 2. That state is drawn to screen
  // 3. Pending input events are processed and made available for the next frame
  while (!window->shouldClose()) {
    cb(inputManager);  // TODO: to be substituted with gamobject Update
    vulkan->draw();
    inputManager->updateEvents();
  }

  destroyWindow();
}

void Application::makeWindow(unsigned int width, unsigned int height) {
  window = make_shared<GlfwWindow>(conf.appName, width, height);
  inputManager = make_shared<InputManager>(*window);
  vulkan = make_unique<VulkanRenderer>(conf, *window);
  window->onResize([this](GLFWwindow*, unsigned int, unsigned int) {
    if (vulkan != nullptr) vulkan->signalResize();
  });
}

void Application::destroyWindow() {
  inputManager = nullptr;
  vulkan = nullptr;
  window = nullptr;
}

const ApplicationConfig& Application::config() const { return conf; }
