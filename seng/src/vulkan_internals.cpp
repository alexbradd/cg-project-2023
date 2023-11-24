#include <vulkan/vulkan.h>

#include <set>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <seng/glfwWindowWrapper.hpp>
#include <seng/log.hpp>
#include <seng/vulkan_internals.hpp>
#include <unordered_set>

using namespace seng;
using namespace seng::internal;
using namespace std;
using namespace vk;

static bool supportsAllLayers(const vector<const char *> &l) {
  const vector<LayerProperties> a = enumerateInstanceLayerProperties();
  return all_of(l.begin(), l.end(), [&a](const char *name) {
    return any_of(a.begin(), a.end(), [&name](const LayerProperties &property) {
      return strcmp(property.layerName, name) == 0;
    });
  });
}

static void addGlfwExtensions(vector<const char *> &ext) {
  uint32_t ext_count = 0;
  const char **glfw_ext = glfwGetRequiredInstanceExtensions(&ext_count);
  for (uint32_t i = 0; i < ext_count; i++) {
    ext.emplace_back(glfw_ext[i]);
  }
}

static bool checkDeviceExtensions(const vector<const char *> &req,
                                  PhysicalDevice dev) {
  vector<ExtensionProperties> available =
      dev.enumerateDeviceExtensionProperties();
  unordered_set<string> required(req.begin(), req.end());
  for (const auto &e : available) required.erase(e.extensionName);
  return required.empty();
}

static bool checkSwapchain(PhysicalDevice dev, SurfaceKHR surface) {
  SwapchainSupportDetails details(dev, surface);
  return !details.formats.empty() && !details.presentModes.empty();
}

DebugMessenger::DebugMessenger(Instance instance) : instance{instance} {}

void DebugMessenger::initialize() {
  DebugUtilsMessengerCreateInfoEXT ci{};
  populateDebugUtilsMessengerCreateInfo(ci);
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

void DebugMessenger::populateDebugUtilsMessengerCreateInfo(
    DebugUtilsMessengerCreateInfoEXT &ci) {
  using Severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
  using Type = vk::DebugUtilsMessageTypeFlagBitsEXT;

  DebugUtilsMessageSeverityFlagsEXT severityFlags(
      Severity::eVerbose | Severity::eWarning | Severity::eError);
  DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
      Type::eGeneral | Type::ePerformance | Type::eValidation);
  ci.messageSeverity = severityFlags;
  ci.messageType = messageTypeFlags;
  ci.pfnUserCallback = debugCallback;
}

VkBool32 DebugMessenger::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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

void DebugMessenger::destroy() {
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
  if (func != nullptr)
    func(static_cast<VkInstance>(instance),
         static_cast<VkDebugUtilsMessengerEXT>(debugMessenger), nullptr);
}

QueueFamilyIndices::QueueFamilyIndices(PhysicalDevice dev, SurfaceKHR surface) {
  vector<QueueFamilyProperties> queueFamilies = dev.getQueueFamilyProperties();

  int i = 0;
  for (const auto &familyProperties : queueFamilies) {
    if (familyProperties.queueFlags & QueueFlagBits::eGraphics)
      graphicsFamily = i;
    if (dev.getSurfaceSupportKHR(i, surface)) presentFamily = i;
    if (isComplete()) break;
    ++i;
  }
}

SwapchainSupportDetails::SwapchainSupportDetails(PhysicalDevice dev,
                                                 SurfaceKHR surface) {
  capabilities = dev.getSurfaceCapabilitiesKHR(surface);
  formats = dev.getSurfaceFormatsKHR(surface);
  presentModes = dev.getSurfacePresentModesKHR(surface);
}

SurfaceFormatKHR SwapchainSupportDetails::chooseFormat() {
  for (const auto &f : formats) {
    if (f.format == Format::eB8G8R8A8Srgb &&
        f.colorSpace == ColorSpaceKHR::eSrgbNonlinear)
      return f;
  }
  return formats[0];
}

Extent2D SwapchainSupportDetails::chooseSwapchainExtent(GLFWwindow *window) {
  if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    Extent2D actual = {static_cast<uint32_t>(width),
                       static_cast<uint32_t>(height)};
    actual.width = clamp(actual.width, capabilities.minImageExtent.width,
                         capabilities.maxImageExtent.width);
    actual.height = clamp(actual.height, capabilities.minImageExtent.height,
                          capabilities.maxImageExtent.height);
    return actual;
  }
}

VulkanInternals::VulkanInternals(Application &app) : app{app} {
  try {
    instance = createInstance();
    if (enableValidationLayers) {
      debugMessenger = DebugMessenger(instance);
      debugMessenger.initialize();
    }
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

Instance VulkanInternals::createInstance() {
  ApplicationInfo ai{};
  ai.pApplicationName = app.getAppName().c_str();
  ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  ai.pEngineName = "seng";
  ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  ai.apiVersion = VK_API_VERSION_1_0;

  if (enableValidationLayers && !supportsAllLayers(validationLayers))
    throw std::runtime_error("Validation layers requested, but not available");

  vector<const char *> extensions{
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};
  if (enableValidationLayers)  // extension always present if validation
                               // layers are present
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  addGlfwExtensions(extensions);

  InstanceCreateInfo ci;
  ci.pApplicationInfo = &ai;
  ci.flags |= InstanceCreateFlagBits::eEnumeratePortabilityKHR;
  ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  ci.ppEnabledExtensionNames = extensions.data();
  if (enableValidationLayers) {
    ci.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    ci.ppEnabledLayerNames = validationLayers.data();
    vk::DebugUtilsMessengerCreateInfoEXT dbg{};
    DebugMessenger::populateDebugUtilsMessengerCreateInfo(dbg);
    ci.pNext = &dbg;
  } else {
    ci.enabledLayerCount = 0;
    ci.pNext = nullptr;
  }

  return vk::createInstance(ci);
}

SurfaceKHR VulkanInternals::createSurface() {
  VkSurfaceKHR surf{};
  auto res =
      glfwCreateWindowSurface(static_cast<VkInstance>(instance),
                              app.getWindow()->getPointer(), nullptr, &surf);
  if (res != VK_SUCCESS)
    throw std::runtime_error("failed to create window surface!");
  return SurfaceKHR(surf);
}

PhysicalDevice VulkanInternals::pickPhysicalDevice() {
  vector<PhysicalDevice> devs = instance.enumeratePhysicalDevices();
  if (devs.empty())
    throw runtime_error("Failed to find GPUs with Vulkan support!");
  auto dev = find_if(devs.begin(), devs.end(), [this](const auto &dev) {
    QueueFamilyIndices queueFamilyIndices(dev, surface);

    bool queueFamilyComplete = queueFamilyIndices.isComplete();
    bool extensionSupported =
        checkDeviceExtensions(requiredDeviceExtensions, dev);
    bool swapchainAdequate =
        extensionSupported ? checkSwapchain(dev, surface) : false;
    return queueFamilyComplete && extensionSupported && swapchainAdequate;
  });
  if (dev == devs.end()) throw runtime_error("Failed to find a suitable GPU!");
  return *dev;
}

pair<Device, Queue> VulkanInternals::createLogicalDeviceAndQueue() {
  QueueFamilyIndices indices(physicalDevice, surface);
  float queuePrio = 1.0f;

  vector<DeviceQueueCreateInfo> qcis;
  set<uint32_t> uniqueQueueFamilies = {*indices.graphicsFamily,
                                       *indices.presentFamily};
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

SwapchainKHR VulkanInternals::createSwapchain() {
  SwapchainSupportDetails details(physicalDevice, surface);

  SurfaceFormatKHR format = details.chooseFormat();
  swapchainFormat = format.format;
  swapchainExtent =
      details.chooseSwapchainExtent(app.getWindow()->getPointer());
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

  QueueFamilyIndices indices(physicalDevice, surface);
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

void VulkanInternals::createImageViews() {
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

VulkanInternals::~VulkanInternals() {
  if (enableValidationLayers) debugMessenger.destroy();
  for (auto &view : swapchainImageViews) device.destroyImageView(view);
  device.destroySwapchainKHR(swapchain);
  device.destroy();
  instance.destroySurfaceKHR(surface);
  instance.destroy();
}
