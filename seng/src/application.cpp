#include <seng/application.hpp>
#include <seng/camera.hpp>
#include <seng/game_context.hpp>
#include <seng/input_manager.hpp>
#include <seng/log.hpp>
#include <seng/rendering/glfw_window.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/rendering/scene.hpp>
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

  ctx = make_unique<GameContext>();
  ctx->_inputManager = make_unique<InputManager>(window.get());

  activeScene = make_unique<Scene>(
      Scene::loadFromDisk(*vulkan, conf, "default", width / static_cast<float>(height)));

  while (!window->shouldClose()) {
    // FIXME: stub drawing
    try {
      const auto start{chrono::high_resolution_clock::now()};
      bool executed =
          vulkan->scopedFrame([&](auto& handle) { activeScene->draw(handle); });
      if (!executed)
        continue;
      else {
        const auto end{chrono::high_resolution_clock::now()};
        ctx->_deltaTime = end - start;
      }
    } catch (const exception& e) {
      log::warning("Unhandled exception reached main loop: {}", e.what());
    }

    ctx->_inputManager->updateEvents();

    cb(ctx.get());  // TODO: to be substituted with gamobject Update
  }

  activeScene = nullptr;
  destroyWindow();
  ctx = nullptr;
}

void Application::makeWindow(unsigned int width, unsigned int height)
{
  window = make_unique<GlfwWindow>(conf.appName, width, height);
  vulkan = make_unique<Renderer>(conf, *window);

  window->onResize([this](GLFWwindow*, unsigned int w, unsigned int h) {
    if (vulkan != nullptr) vulkan->signalResize();
    if (activeScene != nullptr)
      activeScene->mainCamera().aspectRatio(w / static_cast<float>(h));
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
