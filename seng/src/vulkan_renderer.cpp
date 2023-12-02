#include <iterator>
#include <seng/vulkan_renderer.hpp>

using namespace seng::rendering;
using namespace seng::internal;
using namespace std;
using namespace vk::raii;

const std::vector<const char *> VulkanRenderer::requiredDeviceExtensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};
const std::vector<const char *> VulkanRenderer::validationLayers{
    "VK_LAYER_KHRONOS_validation"};

// Intializer functions
static Instance createInstance(Context &, GlfwWindow &);
static bool supportsAllLayers(const vector<const char *> &);

VulkanRenderer::VulkanRenderer(GlfwWindow &window)
    : window(window),
      context(),
      _instance(createInstance(context, window)),
      debugMessenger(_instance, VulkanRenderer::useValidationLayers),
      _surface(window.createVulkanSurface(_instance)),
      device(_instance, _surface) {}

Instance createInstance(Context &context, GlfwWindow &window) {
  vk::ApplicationInfo ai{};
  ai.pApplicationName = window.appName().c_str();
  ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  ai.pEngineName = "seng";
  ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  ai.apiVersion = VK_API_VERSION_1_0;

  if (VulkanRenderer::useValidationLayers &&
      !supportsAllLayers(VulkanRenderer::validationLayers))
    throw runtime_error("Validation layers requested, but not available");

  vector<const char *> extensions{};
  vector<const char *> windowExtensions{window.extensions()};

  extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  extensions.insert(extensions.end(),
                    make_move_iterator(windowExtensions.begin()),
                    make_move_iterator(windowExtensions.end()));
  if (VulkanRenderer::useValidationLayers)
    // extension always present if validation layers are present
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  vk::InstanceCreateInfo ci;
  ci.pApplicationInfo = &ai;
  ci.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
  ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  ci.ppEnabledExtensionNames = extensions.data();
  if (VulkanRenderer::useValidationLayers) {
    ci.enabledLayerCount =
        static_cast<uint32_t>(VulkanRenderer::validationLayers.size());
    ci.ppEnabledLayerNames = VulkanRenderer::validationLayers.data();
    vk::DebugUtilsMessengerCreateInfoEXT dbg{DebugMessenger::createInfo()};
    ci.pNext = &dbg;
  } else {
    ci.enabledLayerCount = 0;
    ci.pNext = nullptr;
  }

  return Instance(context, ci);
}

bool supportsAllLayers(const vector<const char *> &l) {
  const vector<vk::LayerProperties> a = vk::enumerateInstanceLayerProperties();
  return all_of(l.begin(), l.end(), [&](const char *name) {
    return any_of(a.begin(), a.end(), [&](const vk::LayerProperties &property) {
      return strcmp(property.layerName, name) == 0;
    });
  });
}
