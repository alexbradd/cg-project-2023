#include <seng/application.hpp>
#include <seng/input_manager.hpp>
#include <seng/log.hpp>
#include <seng/rendering/glfw_window.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/scene/scene.hpp>
#include <seng/time.hpp>

#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <utility>

using namespace std;
using namespace seng;
using namespace seng::rendering;
using namespace seng::scene;

Application::Application() : Application(ApplicationConfig{}) {}
Application::Application(ApplicationConfig& config) : conf{config} {}
Application::Application(ApplicationConfig&& config) : conf{std::move(config)} {}

void Application::run(unsigned int width, unsigned int height)
{
  glfwWindow = make_unique<GlfwWindow>(conf.appName, width, height);
  glfwWindow->onResize([this](auto, auto, auto) {
    if (vulkan != nullptr) vulkan->signalResize();
  });

  vulkan = make_unique<Renderer>(conf, *glfwWindow);
  inputManager = make_unique<InputManager>(glfwWindow.get());
  activeScene = make_unique<Scene>(Scene(*this));
  activeScene->loadFromDisk("default");

  Timestamp lastFrame = Clock::now();
  while (!glfwWindow->shouldClose()) {
    try {
      bool executed = vulkan->scopedFrame([&](auto& handle) {
        float deltaTime = inSeconds(Clock::now() - lastFrame);

        // Early update
        activeScene->fireEventType(SceneEvents::EARLY_UPDATE, deltaTime);
        inputManager->updateEvents();

        // Update
        activeScene->fireEventType(SceneEvents::UPDATE, deltaTime);

        // Late update
        activeScene->draw(handle);
        activeScene->fireEventType(SceneEvents::LATE_UPDATE, deltaTime);
      });
      if (!executed)
        continue;
      else
        lastFrame = Clock::now();
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
