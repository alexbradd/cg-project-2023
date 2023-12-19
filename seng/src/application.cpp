#include <seng/application.hpp>
#include <seng/camera.hpp>
#include <seng/input_manager.hpp>
#include <seng/log.hpp>
#include <seng/rendering/glfw_window.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/rendering/scene.hpp>
#include <seng/time.hpp>
#include <seng/transform.hpp>

#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <utility>

using namespace std;
using namespace seng;
using namespace seng::rendering;

Application::Application() : Application(ApplicationConfig{}) {}
Application::Application(ApplicationConfig& config) : conf{config} {}
Application::Application(ApplicationConfig&& config) : conf{std::move(config)} {}

void Application::run(unsigned int width,
                      unsigned int height,
                      function<void(float, const Application&)> cb)
{
  glfwWindow = make_unique<GlfwWindow>(conf.appName, width, height);
  glfwWindow->onResize([this](GLFWwindow*, unsigned int w, unsigned int h) {
    if (vulkan != nullptr) vulkan->signalResize();
    if (activeScene != nullptr)
      activeScene->mainCamera().aspectRatio(w / static_cast<float>(h));
  });
  vulkan = make_unique<Renderer>(conf, *glfwWindow);
  inputManager = make_unique<InputManager>(glfwWindow.get());
  activeScene = make_unique<Scene>(
      Scene::loadFromDisk(*vulkan, conf, "default", width / static_cast<float>(height)));

  Timestamp lastFrame = Clock::now();
  while (!glfwWindow->shouldClose()) {
    try {
      const auto delta = Clock::now() - lastFrame;
      bool executed = vulkan->scopedFrame([&](auto& handle) {
        // TODO: pass delta to update func
        activeScene->draw(handle);
      });
      if (!executed)
        continue;
      else
        lastFrame = Clock::now();

      inputManager->updateEvents();

      cb(inSeconds(delta), *this);  // TODO: to be substituted with gamobject Update
    } catch (const exception& e) {
      log::warning("Unhandled exception reached main loop: {}", e.what());
    }
  }

  activeScene = nullptr;
  inputManager = nullptr;
  vulkan = nullptr;
  glfwWindow = nullptr;
}

Application::~Application() = default;
