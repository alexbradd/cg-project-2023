#pragma once

#include <cstdint>
#include <optional>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class QueueFamilyIndices {
 public:
  QueueFamilyIndices(vk::raii::PhysicalDevice &device,
                     vk::raii::SurfaceKHR &surface);

  bool isComplete();
  std::optional<uint32_t> graphicsFamily() { return _graphicsFamily; }
  std::optional<uint32_t> presentFamily() { return _presentFamily; }

 private:
  std::optional<uint32_t> _graphicsFamily;
  std::optional<uint32_t> _presentFamily;
};

}  // namespace seng::rendering
