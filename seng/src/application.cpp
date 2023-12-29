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
  m_glfwWindow = make_unique<GlfwWindow>(conf.appName, width, height);
  m_glfwWindow->onResize([this](auto, auto, auto) {
    if (m_vulkan != nullptr) m_vulkan->signalResize();
  });

  m_vulkan = make_unique<Renderer>(conf, *m_glfwWindow);
  m_inputManager = make_unique<InputManager>(*m_glfwWindow);
  m_scene = make_unique<Scene>(*this);
  m_scene->loadFromDisk("default");

  Timestamp lastFrame = Clock::now();
  while (!m_glfwWindow->shouldClose()) {
    try {
      bool executed = m_vulkan->scopedFrame([&](auto& handle) {
        float deltaTime = inSeconds(Clock::now() - lastFrame);

        // Early update
        m_scene->fireEventType(SceneEvents::EARLY_UPDATE, deltaTime);
        m_inputManager->updateEvents();

        // Update
        m_scene->fireEventType(SceneEvents::UPDATE, deltaTime);

        // Late update
        m_scene->draw(handle);
        m_scene->fireEventType(SceneEvents::LATE_UPDATE, deltaTime);
      });
      if (!executed)
        continue;
      else
        lastFrame = Clock::now();
    } catch (const exception& e) {
      log::warning("Unhandled exception reached main loop: {}", e.what());
    }
  }

  m_scene = nullptr;
  m_inputManager = nullptr;
  m_vulkan = nullptr;
  m_glfwWindow = nullptr;
}

void Application::stop()
{
  if (m_glfwWindow) m_glfwWindow->close();
}

Application::~Application() = default;
