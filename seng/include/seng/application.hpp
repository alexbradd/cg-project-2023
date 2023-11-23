#ifndef __SENG_APPLICATION_HPP__
#define __SENG_APPLICATION_HPP__

#include <memory>
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

  Application &operator=(const Application &other) = delete;
  Application &operator=(const Application &&other) noexcept = delete;

 private:
  std::string appName;
  unsigned int initialWidth, initialHeight;

  class Context;
  std::unique_ptr<Context> ctx;
};

}  // namespace seng

#endif  // __SENG_APPLICATION_HPP__
