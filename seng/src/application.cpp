#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include <memory>
#include <seng/application.hpp>
#include <seng/glfw_window.hpp>
#include <seng/input_manager.hpp>
#include <seng/log.hpp>
#include <seng/vulkan_renderer.hpp>
#include <seng/transform.hpp>

// clang-format off
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// clang-format on

using namespace std;
using namespace seng;
using namespace seng::rendering;

Application::Application() : Application(ApplicationConfig{}) {}
Application::Application(ApplicationConfig& config) : conf{config} {}
Application::Application(ApplicationConfig&& config) :
    conf{std::move(config)} {}
Application::~Application() { destroyWindow(); }

void Application::run(unsigned int width,
                      unsigned int height,
                      function<void(shared_ptr<InputManager>)> cb) {
  glm::mat4 projection = glm::perspective(glm::radians(45.0f), width/static_cast<float>(height), 0.1f, 1000.0f);
  projection[1][1] *= -1;

  Transform cameraTransform(glm::vec3(0.0, 0.0f, 2.0f));
  Transform model;

  makeWindow(width, height);

  // The main applcation loop goes like this:
  //
  // 1. Game state is updated, reacting to input changes
  // 2. That state is drawn to screen
  // 3. Pending input events are processed and made available for the next frame
  while (!window->shouldClose()) {
    cb(inputManager);  // TODO: to be substituted with gamobject Update

    // FIXME: stub drawing
    try {
      vulkan->beginFrame();

      cameraTransform.translate(cameraTransform.forward() * 0.01f);

      glm::mat4 view = glm::inverse(cameraTransform.toMat4());
      vulkan->updateGlobalState(projection, view, glm::vec3(0.0));

      model.rotate(0.0f, 0.0f, 0.01f);
      vulkan->updateModel(model.toMat4());

      vulkan->draw();

      vulkan->endFrame();
    } catch (const BeginFrameException &e) {
      log::info("Could not begin frame: {}", e.what());
    } catch (const exception &e) {
      log::warning("Unhandled exception reached draw function: {}", e.what());
    }

    inputManager->updateEvents();
  }

  destroyWindow();
}

void Application::makeWindow(unsigned int width, unsigned int height) {
  window = make_shared<GlfwWindow>(conf.appName, width, height);
  inputManager = make_shared<InputManager>(*window);
  vulkan = make_unique<VulkanRenderer>(conf, *window);
  window->onResize([this](GLFWwindow*, unsigned int, unsigned int) {
    if (vulkan != nullptr) vulkan->signalResize();
  });
}

void Application::destroyWindow() {
  inputManager = nullptr;
  vulkan = nullptr;
  window = nullptr;
}

const ApplicationConfig& Application::config() const { return conf; }
