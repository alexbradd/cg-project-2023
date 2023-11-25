#ifndef __PRIVATE_SENG_VULKAN_INTERNALS_HPP__
#define __PRIVATE_SENG_VULKAN_INTERNALS_HPP__

// clang-format off
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
// clang-format on

#include <optional>
#include <seng/application.hpp>
#include <unordered_map>
#include <vector>

namespace seng::internal {

struct DebugMessenger {
  vk::Instance instance;
  vk::DebugUtilsMessengerEXT debugMessenger;

  DebugMessenger() = default;
  DebugMessenger(vk::Instance instance);
  ~DebugMessenger() = default;

  void initialize();
  void destroy();

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);
  static void populateDebugUtilsMessengerCreateInfo(
      vk::DebugUtilsMessengerCreateInfoEXT &ci);
};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  QueueFamilyIndices(vk::PhysicalDevice device, vk::SurfaceKHR);

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapchainSupportDetails {
  vk::SurfaceCapabilitiesKHR capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> presentModes;

  SwapchainSupportDetails(vk::PhysicalDevice device, vk::SurfaceKHR surface);

  vk::SurfaceFormatKHR chooseFormat();
  vk::Extent2D chooseSwapchainExtent(GLFWwindow *window);
};

class VulkanInternals {
 public:
  VulkanInternals(Application &app);
  ~VulkanInternals();

  VulkanInternals(const VulkanInternals &) = delete;
  VulkanInternals(const VulkanInternals &&) = delete;

  VulkanInternals &operator=(const VulkanInternals &other) = delete;
  VulkanInternals &operator=(const VulkanInternals &&other) noexcept = delete;

  void loadShadersFromDisk();

 private:
  Application &app;

  vk::Instance instance;
  vk::SurfaceKHR surface;
  vk::PhysicalDevice physicalDevice;
  vk::Device device;
  vk::Queue presentQueue;
  vk::SwapchainKHR swapchain;
  vk::Format swapchainFormat;
  vk::Extent2D swapchainExtent;
  std::vector<vk::Image> swapchainImages;
  std::vector<vk::ImageView> swapchainImageViews;

  std::unordered_map<std::string, vk::ShaderModule> loadedShaders;

  vk::PipelineLayout pipelineLayout;
  vk::RenderPass renderPass;
  vk::Pipeline pipeline;

  const std::vector<const char *> requiredDeviceExtensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  const std::vector<const char *> validationLayers{
      "VK_LAYER_KHRONOS_validation"};

#ifndef NDEBUG
  static constexpr bool enableValidationLayers{true};
#else
  static constexpr bool enableValidationLayers{false};
#endif
  DebugMessenger debugMessenger;

  vk::Instance createInstance();
  vk::SurfaceKHR createSurface();
  vk::PhysicalDevice pickPhysicalDevice();
  std::pair<vk::Device, vk::Queue> createLogicalDeviceAndQueue();
  vk::SwapchainKHR createSwapchain();
  void createImageViews();

  vk::Pipeline createPipeline();
  vk::ShaderModule createShaderModule(const std::vector<char> &code);
  vk::RenderPass createRenderPass();

  void destroyShaders();
};

}  // namespace seng::internal
#endif  // __PRIVATE_SENG_VULKAN_INTERNALS_HPP__
