#pragma once

#include <optional>
#include <seng/application_config.hpp>
#include <seng/time.hpp>

#include <memory>

namespace seng {

namespace rendering {
class GlfwWindow;
class Renderer;
class FrameHandle;
}  // namespace rendering

class Scene;

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

  // Accessors
  const ApplicationConfig &config() const { return conf; }
  const std::unique_ptr<rendering::Renderer> &renderer() const { return m_vulkan; }
  const std::unique_ptr<rendering::GlfwWindow> &window() const { return m_glfwWindow; }
  const std::unique_ptr<Scene> &currentScene() const { return m_scene; }
  const std::unique_ptr<InputManager> &input() const { return m_inputManager; }

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

  /**
   * Switch to the scene with the given name. The switch will be done before the start of
   * the next frame.
   */
  void switchScene(const std::string &name);

 private:
  ApplicationConfig conf;

  std::unique_ptr<rendering::GlfwWindow> m_glfwWindow;
  std::unique_ptr<rendering::Renderer> m_vulkan;
  std::unique_ptr<InputManager> m_inputManager;
  std::unique_ptr<Scene> m_scene;

  std::optional<std::string> m_newSceneName;

  void handleSceneSwitch(rendering::FrameHandle handle);

  Duration frameLimit(Timestamp lastTime, Duration delta) const;
};

}  // namespace seng
