#pragma once

#include <memory>
#include <string>

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
  Application(std::string appName);
  ~Application();

  Application(const Application &) = delete;
  Application(Application &&) = delete;

  /**
   * Starts execution of the engine in a window of the specified starting size.
   * Blocks until application is closed.
   *
   * In case of a fatal error a std::runtime_error will be thrown
   */
  void run(unsigned int width, unsigned int height);

  /**
   * Get the lookup path for SPIR-V shaders
   */
  const std::string &getShaderPath();

  /**
   * Set the current lookup path for SPIR-V shaders
   */
  void setShaderPath(std::string s);

  /**
   * Get the lookup path for models
   */
  const std::string &getModelPath();

  /**
   * Set the current lookup path for models
   */
  void setModelPath(std::string s);

  /**
   * Get the current app name
   */
  const std::string &getAppName();

  Application &operator=(const Application &other) = delete;
  Application &operator=(const Application &&other) noexcept = delete;

 private:
  const std::string appName;

  std::string shaderPath{"./shaders/"};
  std::string modelPath{"./models/"};

  std::shared_ptr<rendering::GlfwWindow> window;
  std::unique_ptr<rendering::VulkanRenderer> vulkan;

  void makeWindow(unsigned int width, unsigned int height);
  void destroyWindow();
};

}  // namespace seng
