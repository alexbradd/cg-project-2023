#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <seng/application.hpp>

using namespace std;
using namespace seng;

class Application::impl {
public:
  std::string w_name;
  unsigned int width, height;

  GLFWwindow* w;
  vk::Instance instance;

  impl(std::string w_name, unsigned int w, unsigned int h) :
    w_name{w_name},
    width{w},
    height{h}
  {}

  void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,
                  GLFW_FALSE); // FIXME: make it resizeable when ready
    this->w = glfwCreateWindow(this->width,
                              this->height,
                              this->w_name.c_str(),
                              nullptr, nullptr);
  }

  void pushGlfwExtensions(std::vector<const char*>& ext) {
    uint32_t ext_count = 0;
    const char ** glfw_ext = glfwGetRequiredInstanceExtensions(&ext_count);
    for (uint32_t i = 0; i < ext_count; i++) {
      ext.emplace_back(glfw_ext[i]);
    }
  }

  void pushMacStupidBullcrap(std::vector<const char *>& ext, vk::InstanceCreateFlags& new_flags) {
    ext.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    new_flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
  }

  vk::Instance createInstance() {
    vk::ApplicationInfo ai {};
    ai.pApplicationName = this->w_name.c_str();
    ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    ai.pEngineName = "seng";
    ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    ai.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char *> extensions {};
    vk::InstanceCreateFlags portability;
    this->pushGlfwExtensions(extensions);
    this->pushMacStupidBullcrap(extensions, portability);

    vk::InstanceCreateInfo ci;
    ci.pApplicationInfo = &ai;
    ci.flags |= portability;
    ci.enabledExtensionCount = (uint32_t) extensions.size();
    ci.ppEnabledExtensionNames = extensions.data();

    return vk::createInstance(ci);
  }

  void initVulkan() {
    try {
      this->instance = this->createInstance();
    } catch (std::exception const & e) {
      throw std::runtime_error("Failed to create instance!");
    }
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(this->w)) {
      glfwPollEvents();
    }
  }

  void cleanup() {
    this->instance.destroy();

    glfwDestroyWindow(this->w);
    glfwTerminate();
  }
};

Application::Application() : Application("Vulkan", 800, 600) {}
Application::Application(std::string window_name,
                               unsigned int width,
                               unsigned int height) :
  pimpl { new impl {window_name, width, height} }
{}
Application::~Application() {}

void Application::run() {
  pimpl->initWindow();
  pimpl->initVulkan();
  pimpl->mainLoop();
  pimpl->cleanup();
}
