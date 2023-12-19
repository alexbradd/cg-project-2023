#pragma once

#include <seng/application_config.hpp>

#include <functional>
#include <memory>

namespace seng {

namespace rendering {
class GlfwWindow;
class Renderer;
class Scene;
}  // namespace rendering

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
   *
   * TODO: For test purposes, we pass in a lambda that will get called every
   * frame, once we have the gameObject system up we will remove this
   */
  void run(unsigned int width,
           unsigned int height,
           std::function<void(float, const Application &)> cb);

  // Accessors
  const ApplicationConfig &config() const { return conf; }
  const std::unique_ptr<rendering::Renderer> &renderer() const { return vulkan; }
  const std::unique_ptr<rendering::GlfwWindow> &window() const { return glfwWindow; }
  const std::unique_ptr<rendering::Scene> &scene() const { return activeScene; }
  const std::unique_ptr<InputManager> &input() const { return inputManager; }

 private:
  ApplicationConfig conf;

  std::unique_ptr<rendering::GlfwWindow> glfwWindow;
  std::unique_ptr<rendering::Renderer> vulkan;
  std::unique_ptr<InputManager> inputManager;
  std::unique_ptr<rendering::Scene> activeScene;
};

}  // namespace seng
