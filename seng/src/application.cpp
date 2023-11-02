#include <cstdint>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <seng/application.hpp>
#include <seng/log.hpp>

using namespace std;
using namespace seng;

VkResult CreateDebugUtilsMessengerEXT(
    vk::Instance instance,
    const vk::DebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const vk::AllocationCallbacks *pAllocator,
    vk::DebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func((VkInstance)instance,
                (const VkDebugUtilsMessengerCreateInfoEXT *)pCreateInfo,
                (const VkAllocationCallbacks *)pAllocator,
                (VkDebugUtilsMessengerEXT *)pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(vk::Instance instance,
                                   vk::DebugUtilsMessengerEXT debugMessenger,
                                   const vk::AllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr)
    func((VkInstance)instance, (VkDebugUtilsMessengerEXT)debugMessenger,
         (const VkAllocationCallbacks *)pAllocator);
}
using namespace vk;

class Application::impl {
 public:
  string w_name;
  unsigned int width, height;

  GLFWwindow *w;
  Instance instance;

#ifndef NDEBUG
  static constexpr bool enableValidationLayers{true};
#else
  static constexpr bool enableValidationLayers{false};
#endif
  DebugUtilsMessengerEXT debugMessenger;

  impl(string w_name, unsigned int w, unsigned int h)
      : w_name{w_name}, width{w}, height{h} {}

  void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,
                   GLFW_FALSE);  // FIXME: make it resizeable when ready
    w = glfwCreateWindow(width, height, w_name.c_str(), nullptr, nullptr);
  }

  void pushGlfwExtensions(vector<const char *> &ext) {
    uint32_t ext_count = 0;
    const char **glfw_ext = glfwGetRequiredInstanceExtensions(&ext_count);
    for (uint32_t i = 0; i < ext_count; i++) {
      ext.emplace_back(glfw_ext[i]);
    }
  }

  void pushMacStupidBullcrap(vector<const char *> &ext,
                             InstanceCreateFlags &new_flags) {
    ext.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    new_flags |= InstanceCreateFlagBits::eEnumeratePortabilityKHR;
  }

  bool supportsAllLayers(const vector<const char *> &l) {
    const vector<LayerProperties> a = enumerateInstanceLayerProperties();
    return all_of(l.begin(), l.end(), [&a](const char *name) {
      return any_of(a.begin(), a.end(),
                    [&name](const LayerProperties &property) {
                      return strcmp(property.layerName, name) == 0;
                    });
    });
  }

  void populateDebugUtilsMessengerCreateInfo(
      DebugUtilsMessengerCreateInfoEXT &ci) {
    DebugUtilsMessageSeverityFlagsEXT severityFlags(
        DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        DebugUtilsMessageSeverityFlagBitsEXT::eError);
    DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    ci.messageSeverity = severityFlags;
    ci.messageType = messageTypeFlags;
    ci.pfnUserCallback = debugCallback;
  }

  Instance createInstance() {
    ApplicationInfo ai{};
    ai.pApplicationName = w_name.c_str();
    ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    ai.pEngineName = "seng";
    ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    ai.apiVersion = VK_API_VERSION_1_0;

    vector<const char *> validationLayers;
    validationLayers.push_back("VK_LAYER_KHRONOS_validation");
    if (enableValidationLayers && !supportsAllLayers(validationLayers)) {
      throw std::runtime_error(
          "Validation layers requested, but not available");
    }

    vector<const char *> extensions{};
    InstanceCreateFlags portability;
    pushGlfwExtensions(extensions);
    pushMacStupidBullcrap(extensions, portability);
    if (enableValidationLayers)  // extension always present if validation
                                 // layers are present
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    InstanceCreateInfo ci;
    ci.pApplicationInfo = &ai;
    ci.flags |= portability;
    ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();
    if (enableValidationLayers) {
      ci.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      ci.ppEnabledLayerNames = validationLayers.data();
      vk::DebugUtilsMessengerCreateInfoEXT dbg{};
      populateDebugUtilsMessengerCreateInfo(dbg);
      ci.pNext = &dbg;
    } else {
      ci.enabledLayerCount = 0;
      ci.pNext = nullptr;
    }

    return vk::createInstance(ci);
  }

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData) {
    switch (messageSeverity) {
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        log::info("Validation layer: {}", pCallbackData->pMessage);
        break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        log::warning("Validation layer: {}", pCallbackData->pMessage);
        break;
      default:
        log::error("Validation layer: {}", pCallbackData->pMessage);
    }
    return VK_FALSE;
  }

  void setupDebugMessager() {
    if (!enableValidationLayers) return;

    DebugUtilsMessengerCreateInfoEXT ci{};
    populateDebugUtilsMessengerCreateInfo(ci);
    if (CreateDebugUtilsMessengerEXT(instance, &ci, nullptr, &debugMessenger) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to set up debug messenger!");
    }
  }

  void initVulkan() {
    try {
      instance = createInstance();
      setupDebugMessager();
    } catch (exception const &e) {
      throw runtime_error("Failed to create instance!");
    }
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(this->w)) {
      glfwPollEvents();
    }
  }

  void cleanup() {
    if (enableValidationLayers)
      DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    instance.destroy();

    glfwDestroyWindow(this->w);
    glfwTerminate();
  }
};

Application::Application() : Application("Vulkan", 800, 600) {}
Application::Application(string window_name, unsigned int width,
                         unsigned int height)
    : pimpl{new impl{window_name, width, height}} {}
Application::~Application() {}

void Application::run() {
  pimpl->initWindow();
  pimpl->initVulkan();
  pimpl->mainLoop();
  pimpl->cleanup();
}
