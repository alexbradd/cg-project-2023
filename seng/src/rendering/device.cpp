#include <seng/log.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/queue_family_indices.hpp>
#include <seng/rendering/swapchain_support_details.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;
using namespace seng::rendering;
using namespace vk::raii;

const vector<const char *> VulkanDevice::REQUIRED_EXT{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static PhysicalDevice pickPhysicalDevice(const Instance &, const SurfaceKHR &);
static bool checkExtensions(const vector<const char *> &, const PhysicalDevice &);
static bool checkSwapchain(const PhysicalDevice &, const SurfaceKHR &);
static Device createLogicalDevice(const PhysicalDevice &, const QueueFamilyIndices &);
static vk::SurfaceFormatKHR detectDepthFormat(const PhysicalDevice &);

VulkanDevice::VulkanDevice(const Instance &instance, const SurfaceKHR &surface) :
    _surface(std::addressof(surface)),
    _physical(pickPhysicalDevice(instance, surface)),
    _queueIndices(_physical, surface),
    _swapchainDetails(_physical, surface),
    _logical(createLogicalDevice(_physical, _queueIndices)),
    _presentQueue(_logical, *_queueIndices.presentFamily(), 0),
    _graphicsQueue(_logical, *_queueIndices.graphicsFamily(), 0),
    _depthFormat(detectDepthFormat(_physical))
{
  log::dbg("Device has beeen created successfully");
}

PhysicalDevice pickPhysicalDevice(const Instance &i, const SurfaceKHR &s)
{
  PhysicalDevices devs(i);
  if (devs.empty()) throw runtime_error("Failed to find GPUs with Vulkan support!");
  auto dev = find_if(devs.begin(), devs.end(), [&](auto &dev) {
    QueueFamilyIndices queueFamilyIndices(dev, s);

    bool queueFamilyComplete = queueFamilyIndices.isComplete();
    bool extensionSupported = checkExtensions(VulkanDevice::REQUIRED_EXT, dev);
    bool swapchainAdequate = extensionSupported ? checkSwapchain(dev, s) : false;
    return queueFamilyComplete && extensionSupported && swapchainAdequate;
  });
  if (dev == devs.end()) throw runtime_error("Failed to find a suitable GPU!");
  return *dev;
}

bool checkExtensions(const vector<const char *> &req, const PhysicalDevice &dev)
{
  vector<vk::ExtensionProperties> available{dev.enumerateDeviceExtensionProperties()};
  unordered_set<string> required(req.begin(), req.end());
  for (const auto &e : available) required.erase(e.extensionName);
  return required.empty();
}

bool checkSwapchain(const PhysicalDevice &dev, const SurfaceKHR &surface)
{
  SwapchainSupportDetails details(dev, surface);
  return !details.formats().empty() && !details.presentModes().empty();
}

Device createLogicalDevice(const PhysicalDevice &phy, const QueueFamilyIndices &indices)
{
  float queuePrio = 1.0f;

  vector<vk::DeviceQueueCreateInfo> qcis;
  set<uint32_t> uniqueQueueFamilies{*indices.graphicsFamily(), *indices.presentFamily()};
  for (auto queueFamily : uniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo qci{};
    qci.queueFamilyIndex = queueFamily;
    qci.setQueuePriorities(queuePrio);
    qcis.emplace_back(qci);
  }

  vk::PhysicalDeviceFeatures features{};

  vk::DeviceCreateInfo dci{};
  dci.setQueueCreateInfos(qcis);
  dci.setPEnabledExtensionNames(VulkanDevice::REQUIRED_EXT);
  dci.pEnabledFeatures = &features;

  return Device(phy, dci);
}

vk::SurfaceFormatKHR detectDepthFormat(const PhysicalDevice &phy)
{
  vector<vk::Format> candidates{vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
                                vk::Format::eD24UnormS8Uint};
  vk::FormatFeatureFlagBits f = vk::FormatFeatureFlagBits::eDepthStencilAttachment;
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
                                       vk::MemoryPropertyFlags flags) const
{
  vk::PhysicalDeviceMemoryProperties props{_physical.getMemoryProperties()};

  for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
    if (filter & (1 << i) && (props.memoryTypes[i].propertyFlags & flags) == flags)
      return i;
  }
  throw runtime_error("Unable to find suitable memory type!");
}

void VulkanDevice::requerySupport()
{
  _queueIndices = QueueFamilyIndices(_physical, *_surface);
  _swapchainDetails = SwapchainSupportDetails(_physical, *_surface);
}

void VulkanDevice::requeryDepthFormat()
{
  _depthFormat = detectDepthFormat(_physical);
}

VulkanDevice::~VulkanDevice()
{
  // Just checking if the device handle is valid is enough
  // since all other handles depend on the existence of it
  if (*_logical != vk::Device{}) log::dbg("Destroying device");
}
