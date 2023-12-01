#pragma once

#include <cstdint>
#include <optional>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

/**
 * The graphics/presentation queue family indexes for a given
 * PhysicalDevice/Surface pair.
 */
class QueueFamilyIndices {
 public:
  /**
   * Instatiate a new class. The given device and surface are queries for
   * support.
   */
  QueueFamilyIndices(vk::raii::PhysicalDevice &device,
                     vk::raii::SurfaceKHR &surface);

  // Accessors
  std::optional<uint32_t> graphicsFamily() { return _graphicsFamily; }
  std::optional<uint32_t> presentFamily() { return _presentFamily; }

  /**
   * Return true if both graphics and presentation indices have been found
   */
  bool isComplete();

 private:
  std::optional<uint32_t> _graphicsFamily;
  std::optional<uint32_t> _presentFamily;
};

}  // namespace seng::rendering
