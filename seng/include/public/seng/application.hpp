#pragma once

#include <memory>
#include <seng/application_config.hpp>

namespace seng {

namespace rendering {
class GlfwWindow;
class VulkanRenderer;
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
   */
  void run(unsigned int width, unsigned int height);

  const ApplicationConfig &config() const;

 private:
  ApplicationConfig conf;

  std::shared_ptr<rendering::GlfwWindow> window;
  std::unique_ptr<rendering::VulkanRenderer> vulkan;

  void makeWindow(unsigned int width, unsigned int height);
  void destroyWindow();
};

}  // namespace seng
