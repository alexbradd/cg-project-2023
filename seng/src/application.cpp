#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED

#include <memory>
#include <seng/application.hpp>
#include <seng/glfw_window.hpp>
#include <seng/input_manager.hpp>
#include <seng/log.hpp>
#include <seng/vulkan_renderer.hpp>

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
  float z = 0.1f;
  glm::mat4 projection = glm::perspective(glm::radians(45.0f), width/static_cast<float>(height), 0.1f, 1000.0f);
  projection[1][1] *= -1;
  makeWindow(width, height);

  // The main applcation loop goes like this:
  //
  // 1. Game state is updated, reacting to input changes
  // 2. That state is drawn to screen
  // 3. Pending input events are processed and made available for the next frame
  while (!window->shouldClose()) {
    cb(inputManager);  // TODO: to be substituted with gamobject Update

    // FIXME: stub drawing
    z += 0.05f;
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, z));
    vulkan->draw(projection, view, glm::vec3(0.0f, 0.0f, 0.0f));

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
