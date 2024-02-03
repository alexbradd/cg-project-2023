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
#include <thread>
#include <utility>

using namespace std;
using namespace seng;
using namespace seng::rendering;

Application::Application() : Application(ApplicationConfig{}) {}
Application::Application(ApplicationConfig& config) : conf{config} {}
Application::Application(ApplicationConfig&& config) : conf{std::move(config)} {}

void Application::run(unsigned int width, unsigned int height)
{
  m_glfwWindow = make_unique<GlfwWindow>(conf.appName, width, height);

  // Don't bother with the token since this callback will live for the
  // lifetime of the window
  m_glfwWindow->onResize().insert([&](auto, auto, auto) {
    if (m_vulkan != nullptr) m_vulkan->signalResize();
  });

  m_vulkan = make_unique<Renderer>(*this, *m_glfwWindow);
  m_inputManager = make_unique<InputManager>(*m_glfwWindow);

  switchScene("default");

  Timestamp completedTime = Clock::now();
  Timestamp lastTime;
  Duration deltaTime;
  while (!m_glfwWindow->shouldClose()) {
    try {
      m_inputManager->updateEvents();
      m_vulkan->scopedFrame([&](auto& handle) {
        lastTime = completedTime;

        // Sample time & frame limit
        completedTime = Clock::now();
        deltaTime = completedTime - lastTime;
        deltaTime = frameLimit(lastTime, deltaTime);

        // Handle scene update
        if (m_newSceneName.has_value()) {
          handleSceneSwitch(handle);
          if (m_scene == nullptr) seng::log::error("No scene loaded");
        } else if (m_scene != nullptr) {
          m_scene->update(deltaTime, handle);
        }
      });
    } catch (const exception& e) {
      log::warning("Unhandled exception reached main loop: {}", e.what());
    }
  }

  m_scene = nullptr;
  m_inputManager = nullptr;
  m_vulkan = nullptr;
  m_glfwWindow = nullptr;
}

Duration Application::frameLimit(Timestamp lastTime, Duration delta) const
{
  auto targetFrameTime = Duration(Clock::period::den / conf.maxFPS);
  if (delta < targetFrameTime) {
    std::this_thread::sleep_for(targetFrameTime - delta);
    return Clock::now() - lastTime;
  }
  return delta;
}

void Application::stop()
{
  if (m_glfwWindow) m_glfwWindow->close();
}

void Application::switchScene(const std::string& name)
{
  m_newSceneName = name;
}

void Application::handleSceneSwitch(FrameHandle handle)
{
  // Dummy frame
  m_vulkan->beginMainRenderPass(handle);
  m_vulkan->endMainRenderPass(handle);

  // Load scene
  m_scene = nullptr;  // destroys the current scene
  auto newScene = Scene::loadFromDisk(*this, *m_newSceneName);
  m_scene = std::move(newScene);
  m_newSceneName.reset();
}

Application::~Application() = default;
