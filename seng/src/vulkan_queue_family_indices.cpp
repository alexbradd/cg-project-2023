#include <seng/vulkan_queue_family_indices.hpp>

using namespace vk::raii;
using namespace seng::rendering;
using namespace std;

QueueFamilyIndices::QueueFamilyIndices(PhysicalDevice &dev,
                                       SurfaceKHR &surface) {
  vector<vk::QueueFamilyProperties> queueFamilies =
      dev.getQueueFamilyProperties();

  int i = 0;
  for (const auto &familyProperties : queueFamilies) {
    if (familyProperties.queueFlags & vk::QueueFlagBits::eGraphics)
      _graphicsFamily = i;
    if (dev.getSurfaceSupportKHR(i, *surface)) _presentFamily = i;
    if (isComplete()) break;
    ++i;
  }
}

bool QueueFamilyIndices::isComplete() {
  return _graphicsFamily.has_value() && _presentFamily.has_value();
}
