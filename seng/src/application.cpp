#include <seng/application.hpp>
#include <seng/camera.hpp>
#include <seng/game_context.hpp>
#include <seng/glfw_window.hpp>
#include <seng/input_manager.hpp>
#include <seng/log.hpp>
#include <seng/transform.hpp>
#include <seng/vulkan_renderer.hpp>

#include <chrono>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <utility>

using namespace std;
using namespace seng;
using namespace seng::rendering;

Application::Application() : Application(ApplicationConfig{}) {}
Application::Application(ApplicationConfig& config) : conf{config} {}
Application::Application(ApplicationConfig&& config) : conf{std::move(config)} {}
Application::~Application()
{
  destroyWindow();
}

void Application::run(unsigned int width,
                      unsigned int height,
                      function<void(shared_ptr<GameContext>)> cb)
{
  makeWindow(width, height);

  Camera camera(width / static_cast<float>(height));
  camera.transform().setPos(0.0, 0.0, 2.0f);
  Transform model;

  ctx = make_shared<GameContext>(camera);
  ctx->_inputManager = make_shared<InputManager>(*window);

  // The main applcation loop goes like this:
  //
  // 1. Game state is updated, reacting to input changes
  // 2. That state is drawn to screen
  // 3. Pending input events are processed and made available for the next frame
  while (!window->shouldClose()) {
    // FIXME: stub drawing
    try {
      const auto start{chrono::high_resolution_clock::now()};
      vulkan->beginFrame();

      vulkan->updateGlobalState(camera.projectionMatrix(), camera.viewMatrix());

      model.rotate(0.0f, 0.0f, 0.01f);
      vulkan->updateModel(model.toMat4());

      vulkan->draw();

      vulkan->endFrame();
      const auto end{chrono::high_resolution_clock::now()};
      ctx->_deltaTime = end - start;
    } catch (const BeginFrameException& e) {
      log::info("Could not begin frame: {}", e.what());
    } catch (const exception& e) {
      log::warning("Unhandled exception reached draw function: {}", e.what());
    }

    ctx->_inputManager->updateEvents();

    cb(ctx);  // TODO: to be substituted with gamobject Update
  }

  destroyWindow();
  ctx = nullptr;
}

void Application::makeWindow(unsigned int width, unsigned int height)
{
  window = make_shared<GlfwWindow>(conf.appName, width, height);
  vulkan = make_unique<VulkanRenderer>(conf, *window);

  window->onResize([this](GLFWwindow*, unsigned int, unsigned int) {
    if (vulkan != nullptr) vulkan->signalResize();
  });
}

void Application::destroyWindow()
{
  vulkan = nullptr;
  window = nullptr;
}

const ApplicationConfig& Application::config() const
{
  return conf;
}
