#include <iterator>
#include <seng/vulkan_renderer.hpp>

using namespace seng::rendering;
using namespace std;
using namespace vk::raii;

const std::vector<const char *> VulkanRenderer::requiredDeviceExtensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};
const std::vector<const char *> VulkanRenderer::validationLayers{
    "VK_LAYER_KHRONOS_validation"};

// Intializer functions
static Instance createInstance(Context &, GlfwWindow &);
static bool supportsAllLayers(const vector<const char *> &);
static CommandPool createCommandPool(VulkanDevice &);
static vector<VulkanFramebuffer> createFramebuffers(VulkanDevice &,
                                                    VulkanSwapchain &,
                                                    VulkanRenderPass &);
static vector<Semaphore> createSemahpores(VulkanDevice &, VulkanSwapchain &);
static vector<VulkanFence> createFences(VulkanDevice &, VulkanSwapchain &);

VulkanRenderer::VulkanRenderer(GlfwWindow &window)
    : window(window),
      context(),
      _instance(createInstance(context, window)),
      debugMessenger(_instance, VulkanRenderer::useValidationLayers),
      _surface(window.createVulkanSurface(_instance)),
      device(_instance, _surface),
      swapchain(device, _surface, window),
      renderPass(device, swapchain),
      cmdPool(createCommandPool(device)),
      graphicsCmdBufs(VulkanCommandBuffer::createMultiple(
          device, cmdPool, swapchain.images().size())),
      framebuffers(createFramebuffers(device, swapchain, renderPass)),
      imageAvailableSems(createSemahpores(device, swapchain)),
      queueCompleteSems(createSemahpores(device, swapchain)),
      inFlightFences(createFences(device, swapchain)),
      imgsInFlight(swapchain.images().size()) {}

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

CommandPool createCommandPool(VulkanDevice &device) {
  return CommandPool(device.logical(),
                     {vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                      device.queueFamiliyIndices().graphicsFamily().value()});
}

vector<VulkanFramebuffer> createFramebuffers(VulkanDevice &dev,
                                             VulkanSwapchain &swap,
                                             VulkanRenderPass &pass) {
  vector<VulkanFramebuffer> fbs{};
  fbs.reserve(swap.images().size());

  for (auto &img : swap.images()) {
    vector<vk::ImageView> attachments(2);  // TODO: make configurable
    attachments[0] = *img;
    attachments[1] = **swap.depthBuffer().imageView();
    fbs.emplace_back(dev, pass, swap.extent(), std::move(attachments));
  }
  return fbs;
}

vector<Semaphore> createSemahpores(VulkanDevice &device,
                                   VulkanSwapchain &swap) {
  vector<Semaphore> ret;
  ret.reserve(swap.images().size());
  for (uint32_t i = 0; i < swap.images().size(); i++)
    ret.emplace_back(device.logical(), vk::SemaphoreCreateInfo{});
  return ret;
}

vector<VulkanFence> createFences(VulkanDevice &device, VulkanSwapchain &swap) {
  vector<VulkanFence> ret;
  ret.reserve(swap.images().size());
  for (uint32_t i = 0; i < swap.images().size(); i++)
    ret.emplace_back(device, true);
  return ret;
}
