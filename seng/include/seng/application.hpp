#pragma once

#include <seng/application_config.hpp>

#include <memory>

namespace seng {

namespace rendering {
class GlfwWindow;
class Renderer;
}  // namespace rendering

namespace scene {
class Scene;
}  // namespace scene

class InputManager;

/**
 * Entry point for user application. Its main role is to bootstrap vulkan and
 * start the application.
 *
 * It is non-copyable and non-moveable.
 */
class Application {
 public:
  Application();
  Application(ApplicationConfig &config);
  Application(ApplicationConfig &&config);
  Application(const Application &) = delete;
  Application(Application &&) = delete;
  ~Application();

  Application &operator=(const Application &other) = delete;
  Application &operator=(const Application &&other) noexcept = delete;

  /**
   * Starts execution of the engine in a window of the specified starting size.
   * Blocks until application is closed.
   *
   * In case of a fatal error a std::runtime_error will be thrown
   */
  void run(unsigned int width, unsigned int height);

  /**
   * Request application exit.
   */
  void stop();

  // Accessors
  const ApplicationConfig &config() const { return conf; }
  const std::unique_ptr<rendering::Renderer> &renderer() const { return m_vulkan; }
  const std::unique_ptr<rendering::GlfwWindow> &window() const { return m_glfwWindow; }
  const std::unique_ptr<scene::Scene> &currentScene() const { return m_scene; }
  const std::unique_ptr<InputManager> &input() const { return m_inputManager; }

 private:
  ApplicationConfig conf;

  std::unique_ptr<rendering::GlfwWindow> m_glfwWindow;
  std::unique_ptr<rendering::Renderer> m_vulkan;
  std::unique_ptr<InputManager> m_inputManager;
  std::unique_ptr<scene::Scene> m_scene;
};

}  // namespace seng
