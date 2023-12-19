#include <seng/application.hpp>
#include <seng/camera.hpp>
#include <seng/game_context.hpp>
#include <seng/input_manager.hpp>
#include <seng/log.hpp>
#include <seng/rendering/glfw_window.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/transform.hpp>

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
                      function<void(const GameContext*)> cb)
{
  makeWindow(width, height);

  Camera camera(width / static_cast<float>(height));
  camera.transform().setPos(0.0, 0.0, 2.0f);
  Transform model;

  ctx = make_unique<GameContext>();
  ctx->_inputManager = make_unique<InputManager>(window.get());

  // The main applcation loop goes like this:
  //
  // 1. Game state is updated, reacting to input changes
  // 2. That state is drawn to screen
  // 3. Pending input events are processed and made available for the next frame
  while (!window->shouldClose()) {
    // FIXME: stub drawing
    try {
      const auto start{chrono::high_resolution_clock::now()};

      auto maybeFrameHandle = vulkan->beginFrame();
      if (!maybeFrameHandle.has_value()) continue;
      auto& frameHandle = *maybeFrameHandle;

      vulkan->updateGlobalState(camera.projectionMatrix(), camera.viewMatrix());

      model.rotate(0.0f, 0.0f, 0.01f);
      vulkan->updateModel(model.toMat4());

      vulkan->draw();

      vulkan->endFrame(frameHandle);
      const auto end{chrono::high_resolution_clock::now()};
      ctx->_deltaTime = end - start;
    } catch (const exception& e) {
      log::warning("Unhandled exception reached draw function: {}", e.what());
    }

    ctx->_inputManager->updateEvents();

    cb(ctx.get());  // TODO: to be substituted with gamobject Update
  }

  destroyWindow();
  ctx = nullptr;
}

void Application::makeWindow(unsigned int width, unsigned int height)
{
  window = make_unique<GlfwWindow>(conf.appName, width, height);
  vulkan = make_unique<Renderer>(conf, *window);

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
