#pragma once

#include <seng/application_config.hpp>

#include <functional>
#include <memory>

namespace seng {

class GameContext;

namespace rendering {
class GlfwWindow;
class Renderer;
class Scene;
}  // namespace rendering

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
           std::function<void(const GameContext *)> cb);

  const ApplicationConfig &config() const;

 private:
  ApplicationConfig conf;

  std::shared_ptr<rendering::GlfwWindow> window;
  std::unique_ptr<rendering::Renderer> vulkan;
  std::unique_ptr<rendering::Scene> activeScene;

  std::shared_ptr<GameContext> ctx;

  void makeWindow(unsigned int width, unsigned int height);
  void destroyWindow();
};

}  // namespace seng
