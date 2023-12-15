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
  QueueFamilyIndices(const vk::raii::PhysicalDevice &device,
                     const vk::raii::SurfaceKHR &surface);

  // Accessors
  const std::optional<uint32_t> graphicsFamily() const { return _graphicsFamily; }
  const std::optional<uint32_t> presentFamily() const { return _presentFamily; }

  /**
   * Return true if both graphics and presentation indices have been found
   */
  bool isComplete() const;

 private:
  std::optional<uint32_t> _graphicsFamily;
  std::optional<uint32_t> _presentFamily;
};

}  // namespace seng::rendering
