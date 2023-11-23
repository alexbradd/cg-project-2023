#include <cstdint>
#include <iostream>
#include <iterator>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <seng/application.hpp>
#include <seng/log.hpp>

using namespace std;
using namespace seng;
using namespace vk;

class Application::impl {
 public:
  string w_name;
  unsigned int width, height;

  GLFWwindow *window;
  Instance instance;
  SurfaceKHR surface;

  PhysicalDevice physicalDevice;
  Device device;
  Queue presentQueue;
  SwapchainKHR swapchain;
  Format swapchainFormat;
  Extent2D swapchainExtent;
  vector<Image> swapchainImages;
  vector<ImageView> swapchainImageViews;

  const vector<const char *> requiredDeviceExtensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  struct QueueFamilyIndices {
    optional<uint32_t> graphicsFamily;
    optional<uint32_t> presentFamily;

    bool isComplete() {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }
  };

  struct SwapchainSupportDetails {
    SurfaceCapabilitiesKHR capabilities;
    vector<SurfaceFormatKHR> formats;
    vector<PresentModeKHR> presentModes;
  };

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
    window = glfwCreateWindow(width, height, w_name.c_str(), nullptr, nullptr);
  }

  void initVulkan() {
    try {
      instance = createInstance();
      if (enableValidationLayers) setupDebugMessager();
      surface = createSurface();
      physicalDevice = pickPhysicalDevice();
      tie(device, presentQueue) = createLogicalDeviceAndQueue();
      swapchain = createSwapchain();
      swapchainImages = device.getSwapchainImagesKHR(swapchain);
      createImageViews();
    } catch (exception const &e) {
      log::error(e.what());
      log::error("Failed to create instance!");
      throw runtime_error("Failed to create instance!");
    }
  }

  Instance createInstance() {
    ApplicationInfo ai{};
    ai.pApplicationName = w_name.c_str();
    ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    ai.pEngineName = "seng";
    ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    ai.apiVersion = VK_API_VERSION_1_0;

    vector<const char *> validationLayers{"VK_LAYER_KHRONOS_validation"};
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

  bool supportsAllLayers(const vector<const char *> &l) {
    const vector<LayerProperties> a = enumerateInstanceLayerProperties();
    return all_of(l.begin(), l.end(), [&a](const char *name) {
      return any_of(a.begin(), a.end(),
                    [&name](const LayerProperties &property) {
                      return strcmp(property.layerName, name) == 0;
                    });
    });
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

  void setupDebugMessager() {
    DebugUtilsMessengerCreateInfoEXT ci{};
    populateDebugUtilsMessengerCreateInfo(ci);
    createDebugUtilsMessengerEXT(ci);
  }

  void createDebugUtilsMessengerEXT(
      const DebugUtilsMessengerCreateInfoEXT &ci) {
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr) {
      func(static_cast<VkInstance>(instance),
           reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT *>(&ci),
           nullptr,
           reinterpret_cast<VkDebugUtilsMessengerEXT *>(&debugMessenger));
    } else {
      throw runtime_error("failed to set up debug messenger!");
    }
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

  SurfaceKHR createSurface() {
    VkSurfaceKHR surf{};
    auto res = glfwCreateWindowSurface(static_cast<VkInstance>(instance),
                                       window, nullptr, &surf);
    if (res != VK_SUCCESS)
      throw std::runtime_error("failed to create window surface!");
    return SurfaceKHR(surf);
  }

  PhysicalDevice pickPhysicalDevice() {
    vector<PhysicalDevice> devs = instance.enumeratePhysicalDevices();
    if (devs.empty())
      throw runtime_error("Failed to find GPUs with Vulkan support!");
    auto dev = find_if(devs.begin(), devs.end(), [this](const auto &dev) {
      auto queueFamilyComplete = findQueueFamilies(dev).isComplete();
      auto extensionSupported = checkDeviceExtensions(dev);
      auto swapchainAdequate =
          extensionSupported ? checkSwapchain(dev, surface) : false;
      return queueFamilyComplete && extensionSupported && swapchainAdequate;
    });
    if (dev == devs.end())
      throw runtime_error("Failed to find a suitable GPU!");
    return *dev;
  }

  QueueFamilyIndices findQueueFamilies(PhysicalDevice dev) {
    vector<QueueFamilyProperties> queueFamilies =
        dev.getQueueFamilyProperties();
    QueueFamilyIndices indices{};

    int i = 0;
    for (const auto &familyProperties : queueFamilies) {
      if (familyProperties.queueFlags & QueueFlagBits::eGraphics)
        indices.graphicsFamily = i;
      if (dev.getSurfaceSupportKHR(i, surface)) indices.presentFamily = i;
      if (indices.isComplete()) break;
      ++i;
    }

    return indices;
  }

  bool checkDeviceExtensions(PhysicalDevice dev) {
    vector<ExtensionProperties> available =
        dev.enumerateDeviceExtensionProperties();
    unordered_set<string> required(requiredDeviceExtensions.begin(),
                                   requiredDeviceExtensions.end());
    for (const auto &e : available) required.erase(e.extensionName);
    return required.empty();
  }

  bool checkSwapchain(PhysicalDevice dev, SurfaceKHR surface) {
    SwapchainSupportDetails details = querySwapchainDetails(dev, surface);
    return !details.formats.empty() && !details.presentModes.empty();
  }

  SwapchainSupportDetails querySwapchainDetails(PhysicalDevice dev,
                                                SurfaceKHR surface) {
    SwapchainSupportDetails details;
    details.capabilities = dev.getSurfaceCapabilitiesKHR(surface);
    details.formats = dev.getSurfaceFormatsKHR(surface);
    details.presentModes = dev.getSurfacePresentModesKHR(surface);
    return details;
  }

  pair<Device, Queue> createLogicalDeviceAndQueue() {
    auto indices = findQueueFamilies(physicalDevice);
    float queuePrio = 1.0f;

    vector<DeviceQueueCreateInfo> qcis;
    set<uint32_t> uniqueQueueFamilies = {*(indices.graphicsFamily),
                                         *(indices.presentFamily)};
    for (auto queueFamily : uniqueQueueFamilies) {
      DeviceQueueCreateInfo qci{};
      qci.queueFamilyIndex = queueFamily;
      qci.queueCount = 1;
      qci.pQueuePriorities = &queuePrio;
      qcis.push_back(qci);
    }

    PhysicalDeviceFeatures features{};

    DeviceCreateInfo dci{};
    dci.pQueueCreateInfos = qcis.data();
    dci.queueCreateInfoCount = static_cast<uint32_t>(qcis.size());
    dci.enabledExtensionCount =
        static_cast<uint32_t>(requiredDeviceExtensions.size());
    dci.ppEnabledExtensionNames = requiredDeviceExtensions.data();
    dci.pEnabledFeatures = &features;

    Device d = physicalDevice.createDevice(dci);
    Queue q = d.getQueue(*(indices.presentFamily), 0);

    return pair{d, q};
  }

  SwapchainKHR createSwapchain() {
    SwapchainSupportDetails details =
        querySwapchainDetails(physicalDevice, surface);

    SurfaceFormatKHR format = chooseSwapchainFormat(details.formats);
    swapchainFormat = format.format;
    swapchainExtent = chooseSwapchainExtent(details.capabilities);
    PresentModeKHR presentMode = PresentModeKHR::eFifo;
    uint32_t imageCount = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 &&
        imageCount > details.capabilities.maxImageCount)
      imageCount = details.capabilities.maxImageCount;

    SwapchainCreateInfoKHR sci{};
    sci.surface = surface;
    sci.minImageCount = imageCount;
    sci.imageFormat = format.format;
    sci.imageColorSpace = format.colorSpace;
    sci.imageExtent = swapchainExtent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = ImageUsageFlagBits::eColorAttachment;
    sci.preTransform = details.capabilities.currentTransform;
    sci.compositeAlpha = CompositeAlphaFlagBitsKHR::eOpaque;
    sci.presentMode = presentMode;
    sci.clipped = true;
    sci.oldSwapchain = VK_NULL_HANDLE;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    if (indices.graphicsFamily != indices.presentFamily) {
      uint32_t queueFamilyIndices[] = {*indices.graphicsFamily,
                                       *indices.presentFamily};
      sci.imageSharingMode = SharingMode::eConcurrent;
      sci.queueFamilyIndexCount = 2;
      sci.pQueueFamilyIndices = queueFamilyIndices;
    } else {
      sci.imageSharingMode = SharingMode::eExclusive;
      sci.queueFamilyIndexCount = 0;
      sci.pQueueFamilyIndices = nullptr;
    }

    return device.createSwapchainKHR(sci);
  }

  SurfaceFormatKHR chooseSwapchainFormat(
      const vector<SurfaceFormatKHR> &available) {
    for (const auto &f : available) {
      if (f.format == Format::eB8G8R8A8Srgb &&
          f.colorSpace == ColorSpaceKHR::eSrgbNonlinear)
        return f;
    }
    return available[0];
  }

  Extent2D chooseSwapchainExtent(const SurfaceCapabilitiesKHR &caps) {
    if (caps.currentExtent.width != numeric_limits<uint32_t>::max()) {
      return caps.currentExtent;
    } else {
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);

      Extent2D actual = {static_cast<uint32_t>(width),
                         static_cast<uint32_t>(height)};
      actual.width = clamp(actual.width, caps.minImageExtent.width,
                           caps.maxImageExtent.width);
      actual.height = clamp(actual.height, caps.minImageExtent.height,
                            caps.maxImageExtent.height);
      return actual;
    }
  }

  void createImageViews() {
    swapchainImageViews.resize(swapchainImages.size());
    for (const auto &i : swapchainImages) {
      ImageViewCreateInfo ci;
      ci.image = i;
      ci.viewType = ImageViewType::e2D;
      ci.format = swapchainFormat;
      ci.components.r = ComponentSwizzle::eIdentity;
      ci.components.g = ComponentSwizzle::eIdentity;
      ci.components.b = ComponentSwizzle::eIdentity;
      ci.components.a = ComponentSwizzle::eIdentity;
      ci.subresourceRange.aspectMask = ImageAspectFlagBits::eColor;
      ci.subresourceRange.baseMipLevel = 0;
      ci.subresourceRange.levelCount = 1;
      ci.subresourceRange.baseArrayLayer = 0;
      ci.subresourceRange.layerCount = 1;

      swapchainImageViews.push_back(device.createImageView(ci));
    }
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    }
  }

  void cleanup() {
    if (enableValidationLayers) destroyDebugUtilsMessengerEXT();
    for (auto &view : swapchainImageViews) device.destroyImageView(view);
    device.destroySwapchainKHR(swapchain);
    device.destroy();
    instance.destroySurfaceKHR(surface);
    instance.destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
  }

  void destroyDebugUtilsMessengerEXT() {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr)
      func(static_cast<VkInstance>(instance),
           static_cast<VkDebugUtilsMessengerEXT>(debugMessenger), nullptr);
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
