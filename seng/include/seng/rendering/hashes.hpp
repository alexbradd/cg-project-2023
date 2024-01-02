#pragma once

#include <seng/utils.hpp>

#include <vulkan/vulkan.hpp>

// Adapted from Vulkan-Samples. Needed so that we can put a DescriptorSetLayout
// handle in a hashmap
MAKE_HASHABLE(vk::DescriptorSetLayout, static_cast<VkDescriptorSetLayout>(t));
