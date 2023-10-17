#ifndef __SENG_APPLICATION_HPP__
#define __SENG_APPLICATION_HPP__

#include <GLFW/glfw3.h>
#include <string>
namespace seng {

/**
 * Entry point for user application. Its main role is to bootstrap vulkan and
 * start the application.
 *
 * It is non-copyable and non-moveable.
 */
class Application {
public:
  Application() : Application("Vulkan", 800, 600)
  {}

  Application(std::string window_name,
              unsigned int width,
              unsigned int height) :
    w_name { window_name },
    width { width },
    height { height }
  {}

  Application(const Application &) = delete;
  Application(const Application &&) = delete;

  /**
   * Starts execution of the engine. Blocks until application is closed.
   *
   * In case of a fatal error a std::runtime_error will be thrown
   */
  void run();

  Application &operator=(const Application &other) = delete;
  Application &operator=(const Application &&other) noexcept = delete;

private:
  std::string w_name;
  unsigned int width, height;

  GLFWwindow* w;

  void initWindow();
  void initVulkan();
  void mainLoop();
  void cleanup();
};

} // namespace seng

#endif // __SENG_APPLICATION_HPP__
