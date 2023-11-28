#ifndef __SENG_APPLICATION_HPP__
#define __SENG_APPLICATION_HPP__

#include <memory>
#include <string>
namespace seng {
namespace internal {
class GlfwWindowWrapper;
class VulkanInternals;
}  // namespace internal

/**
 * Entry point for user application. Its main role is to bootstrap vulkan and
 * start the application.
 *
 * It is non-copyable and non-moveable.
 */
class Application {
 public:
  Application();
  Application(std::string appName, unsigned int width, unsigned int height);
  ~Application();

  Application(const Application &) = delete;
  Application(const Application &&) = delete;

  /**
   * Starts execution of the engine. Blocks until application is closed.
   *
   * In case of a fatal error a std::runtime_error will be thrown
   */
  void run();

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
   * Get the internal GLFWwindow wrapper
   */
  std::shared_ptr<internal::GlfwWindowWrapper> getWindow();

  /**
   * Get the current app name
   */
  const std::string &getAppName();

  Application &operator=(const Application &other) = delete;
  Application &operator=(const Application &&other) noexcept = delete;

 private:
  const std::string appName;
  const unsigned int initialWidth, initialHeight;

  std::string shaderPath{"./shaders/"};
  std::string modelPath{"./models/"};

  std::shared_ptr<internal::GlfwWindowWrapper> window;
  std::shared_ptr<internal::VulkanInternals> vulkan;

  void makeWindow();
  void destroyWindow();
};

}  // namespace seng

#endif  // __SENG_APPLICATION_HPP__
