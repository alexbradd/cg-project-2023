#include <seng/log.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/glfw_window.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;
using namespace seng::rendering;

// QueueFamilyIndices
QueueFamilyIndices::QueueFamilyIndices(const vk::raii::PhysicalDevice &dev,
                                       const vk::raii::SurfaceKHR &surface)
{
  vector<vk::QueueFamilyProperties> queueFamilies = dev.getQueueFamilyProperties();

  int i = 0;
  for (const auto &familyProperties : queueFamilies) {
    if (familyProperties.queueFlags & vk::QueueFlagBits::eGraphics) graphicsFamily = i;
    if (dev.getSurfaceSupportKHR(i, *surface)) presentFamily = i;
    if (isComplete()) break;
    ++i;
  }
}

bool QueueFamilyIndices::isComplete() const
{
  return graphicsFamily.has_value() && presentFamily.has_value();
}

// SwapchainSupportDetails
SwapchainSupportDetails::SwapchainSupportDetails(const vk::raii::PhysicalDevice &dev,
                                                 const vk::raii::SurfaceKHR &surface) :
    capabilities(dev.getSurfaceCapabilitiesKHR(*surface)),
    formats(dev.getSurfaceFormatsKHR(*surface)),
    presentModes(dev.getSurfacePresentModesKHR(*surface))
{
}

vk::SurfaceFormatKHR SwapchainSupportDetails::chooseFormat() const
{
  for (const auto &f : formats) {
    if (f.format == vk::Format::eB8G8R8A8Srgb &&
        f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
      return f;
  }
  return formats[0];
}

vk::Extent2D SwapchainSupportDetails::chooseExtent(const GlfwWindow &window) const
{
  if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    auto size = window.framebufferSize();

    vk::Extent2D actual = {static_cast<uint32_t>(size.first),
                           static_cast<uint32_t>(size.second)};
    actual.width = clamp(actual.width, capabilities.minImageExtent.width,
                         capabilities.maxImageExtent.width);
    actual.height = clamp(actual.height, capabilities.minImageExtent.height,
                          capabilities.maxImageExtent.height);
    return actual;
  }
}

const vector<const char *> Device::REQUIRED_EXT{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static vk::raii::PhysicalDevice pickPhysicalDevice(const vk::raii::Instance &,
                                                   const vk::raii::SurfaceKHR &);
static bool checkExtensions(const vector<const char *> &,
                            const vk::raii::PhysicalDevice &);
static bool checkSwapchain(const vk::raii::PhysicalDevice &,
                           const vk::raii::SurfaceKHR &);
static vk::raii::Device createLogicalDevice(const vk::raii::PhysicalDevice &,
                                            const QueueFamilyIndices &);
static vk::SurfaceFormatKHR detectDepthFormat(const vk::raii::PhysicalDevice &);

Device::Device(const vk::raii::Instance &instance, const vk::raii::SurfaceKHR &surface) :
    m_surface(std::addressof(surface)),
    m_physical(pickPhysicalDevice(instance, surface)),
    m_queueIndices(m_physical, surface),
    m_swapDetails(m_physical, surface),
    m_logical(createLogicalDevice(m_physical, m_queueIndices)),
    m_presentQueue(m_logical, *m_queueIndices.presentFamily, 0),
    m_graphicsQueue(m_logical, *m_queueIndices.graphicsFamily, 0),
    m_depthFormat(detectDepthFormat(m_physical))
{
  log::dbg("Device has beeen created successfully");
}

vk::raii::PhysicalDevice pickPhysicalDevice(const vk::raii::Instance &i,
                                            const vk::raii::SurfaceKHR &s)
{
  vk::raii::PhysicalDevices devs(i);
  if (devs.empty()) throw runtime_error("Failed to find GPUs with Vulkan support!");
  auto dev = find_if(devs.begin(), devs.end(), [&](auto &dev) {
    QueueFamilyIndices queueFamilyIndices(dev, s);

    bool queueFamilyComplete = queueFamilyIndices.isComplete();
    bool extensionSupported = checkExtensions(Device::REQUIRED_EXT, dev);
    bool swapchainAdequate = extensionSupported ? checkSwapchain(dev, s) : false;
    return queueFamilyComplete && extensionSupported && swapchainAdequate;
  });
  if (dev == devs.end()) throw runtime_error("Failed to find a suitable GPU!");
  return *dev;
}

bool checkExtensions(const vector<const char *> &req, const vk::raii::PhysicalDevice &dev)
{
  vector<vk::ExtensionProperties> available{dev.enumerateDeviceExtensionProperties()};
  unordered_set<string> required(req.begin(), req.end());
  for (const auto &e : available) required.erase(e.extensionName);
  return required.empty();
}

bool checkSwapchain(const vk::raii::PhysicalDevice &dev,
                    const vk::raii::SurfaceKHR &surface)
{
  SwapchainSupportDetails details(dev, surface);
  return !details.formats.empty() && !details.presentModes.empty();
}

vk::raii::Device createLogicalDevice(const vk::raii::PhysicalDevice &phy,
                                     const QueueFamilyIndices &indices)
{
  float queuePrio = 1.0f;

  vector<vk::DeviceQueueCreateInfo> qcis;
  set<uint32_t> uniqueQueueFamilies{*indices.graphicsFamily, *indices.presentFamily};
  for (auto queueFamily : uniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo qci{};
    qci.queueFamilyIndex = queueFamily;
    qci.setQueuePriorities(queuePrio);
    qcis.emplace_back(qci);
  }

  vk::PhysicalDeviceFeatures features{};

  vk::DeviceCreateInfo dci{};
  dci.setQueueCreateInfos(qcis);
  dci.setPEnabledExtensionNames(Device::REQUIRED_EXT);
  dci.pEnabledFeatures = &features;

  return vk::raii::Device(phy, dci);
}

vk::SurfaceFormatKHR detectDepthFormat(const vk::raii::PhysicalDevice &phy)
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

uint32_t Device::findMemoryIndex(uint32_t filter, vk::MemoryPropertyFlags flags) const
{
  vk::PhysicalDeviceMemoryProperties props{m_physical.getMemoryProperties()};

  for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
    if (filter & (1 << i) && (props.memoryTypes[i].propertyFlags & flags) == flags)
      return i;
  }
  throw runtime_error("Unable to find suitable memory type!");
}

void Device::requerySupport()
{
  m_queueIndices = QueueFamilyIndices(m_physical, *m_surface);
  m_swapDetails = SwapchainSupportDetails(m_physical, *m_surface);
}

void Device::requeryDepthFormat()
{
  m_depthFormat = detectDepthFormat(m_physical);
}

Device::~Device()
{
  // Just checking if the device handle is valid is enough
  // since all other handles depend on the existence of it
  if (*m_logical != vk::Device{}) log::dbg("Destroying device");
}
