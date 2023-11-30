#include <seng/vulkan_device.hpp>
#include <seng/vulkan_renderer.hpp>
#include <set>
#include <stdexcept>
#include <unordered_set>

using namespace std;
using namespace seng::rendering;
using namespace vk::raii;

static PhysicalDevice pickPhysicalDevice(Instance &, SurfaceKHR &);
static bool checkExtensions(const vector<const char *> &, PhysicalDevice &);
static bool checkSwapchain(PhysicalDevice &, SurfaceKHR &);
static Device createLogicalDevice(PhysicalDevice &, QueueFamilyIndices &);
static vk::SurfaceFormatKHR detectDepthFormat(PhysicalDevice &);

VulkanDevice::VulkanDevice(Instance &instance, SurfaceKHR &surface)
    : _surface(surface),
      _physical(pickPhysicalDevice(instance, surface)),
      _queueIndices(_physical, surface),
      _swapchainDetails(_physical, surface),
      _logical(createLogicalDevice(_physical, _queueIndices)),
      _presentQueue(_logical, *_queueIndices.presentFamily(), 0),
      _graphicsQueue(_logical, *_queueIndices.graphicsFamily(), 0),
      _depthFormat(detectDepthFormat(_physical)) {}

PhysicalDevice pickPhysicalDevice(Instance &i, SurfaceKHR &s) {
  PhysicalDevices devs(i);
  if (devs.empty())
    throw runtime_error("Failed to find GPUs with Vulkan support!");
  auto dev = find_if(devs.begin(), devs.end(), [&](auto &dev) {
    QueueFamilyIndices queueFamilyIndices(dev, s);

    bool queueFamilyComplete = queueFamilyIndices.isComplete();
    bool extensionSupported =
        checkExtensions(VulkanRenderer::requiredDeviceExtensions, dev);
    bool swapchainAdequate =
        extensionSupported ? checkSwapchain(dev, s) : false;
    return queueFamilyComplete && extensionSupported && swapchainAdequate;
  });
  if (dev == devs.end()) throw runtime_error("Failed to find a suitable GPU!");
  return *dev;
}

bool checkExtensions(const vector<const char *> &req, PhysicalDevice &dev) {
  vector<vk::ExtensionProperties> available{
      dev.enumerateDeviceExtensionProperties()};
  unordered_set<string> required(req.begin(), req.end());
  for (const auto &e : available) required.erase(e.extensionName);
  return required.empty();
}

bool checkSwapchain(PhysicalDevice &dev, SurfaceKHR &surface) {
  SwapchainSupportDetails details(dev, surface);
  return !details.formats().empty() && !details.presentModes().empty();
}

Device createLogicalDevice(PhysicalDevice &phy, QueueFamilyIndices &indices) {
  float queuePrio = 1.0f;

  vector<vk::DeviceQueueCreateInfo> qcis;
  set<uint32_t> uniqueQueueFamilies{*indices.graphicsFamily(),
                                    *indices.presentFamily()};
  for (auto queueFamily : uniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo qci{};
    qci.queueFamilyIndex = queueFamily;
    qci.setQueuePriorities(queuePrio);
    qcis.emplace_back(qci);
  }

  vk::PhysicalDeviceFeatures features{};

  vk::DeviceCreateInfo dci{};
  dci.setQueueCreateInfos(qcis);
  dci.setPEnabledExtensionNames(VulkanRenderer::requiredDeviceExtensions);
  dci.pEnabledFeatures = &features;

  return Device(phy, dci);
}

vk::SurfaceFormatKHR detectDepthFormat(PhysicalDevice &phy) {
  vector<vk::Format> candidates{vk::Format::eD32Sfloat,
                                vk::Format::eD32SfloatS8Uint,
                                vk::Format::eD24UnormS8Uint};
  vk::FormatFeatureFlagBits f =
      vk::FormatFeatureFlagBits::eDepthStencilAttachment;
  for (auto format : candidates) {
    vk::FormatProperties props = phy.getFormatProperties(format);
    if ((props.linearTilingFeatures & f) == f) {
      return format;
    } else if ((props.optimalTilingFeatures & f) == f) {
      return format;
    }
  }
  throw runtime_error("Unable to find appropriate depth format!");
}

uint32_t VulkanDevice::findMemoryIndex(uint32_t filter,
                                       vk::MemoryPropertyFlags flags) {
  vk::PhysicalDeviceMemoryProperties props{_physical.getMemoryProperties()};

  for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
    if (filter & (1 << i) &&
        (props.memoryTypes[i].propertyFlags & flags) == flags)
      return i;
  }
  throw runtime_error("Unable to find suitable memory type!");
}

void VulkanDevice::requerySupport() {
  _queueIndices = QueueFamilyIndices(_physical, _surface);
  _swapchainDetails = SwapchainSupportDetails(_physical, _surface);
}

void VulkanDevice::requeryDepthFormat() {
  _depthFormat = detectDepthFormat(_physical);
}
