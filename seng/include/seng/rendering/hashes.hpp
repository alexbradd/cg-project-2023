#pragma once

#include <seng/utils.hpp>

#include <vulkan/vulkan.hpp>

// Copied from Vulkan-Samples. Needed so that we can put a DescriptorSetLayout handle in a
// hashmap
namespace std {
template <>
struct hash<vk::DescriptorSetLayout> {
  std::size_t operator()(const vk::DescriptorSetLayout &descriptorSetLayout) const
  {
    std::size_t result = 0;
    seng::internal::hashCombine(result,
                                static_cast<VkDescriptorSetLayout>(descriptorSetLayout));
    return result;
  }
};
}  // namespace std
