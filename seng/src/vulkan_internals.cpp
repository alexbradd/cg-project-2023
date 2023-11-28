#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <seng/glfwWindowWrapper.hpp>
#include <seng/log.hpp>
#include <seng/utils.hpp>
#include <seng/vulkan_internals.hpp>
#include <set>
#include <stdexcept>
#include <unordered_set>

using namespace seng;
using namespace seng::internal;
using namespace std;
using namespace vk;

static bool supportsAllLayers(const vector<const char *> &l) {
  const vector<LayerProperties> a = enumerateInstanceLayerProperties();
  return all_of(l.begin(), l.end(), [&a](const char *name) {
    return any_of(a.begin(), a.end(), [&name](const LayerProperties &property) {
      return strcmp(property.layerName, name) == 0;
    });
  });
}

static void addGlfwExtensions(vector<const char *> &ext) {
  uint32_t ext_count = 0;
  const char **glfw_ext = glfwGetRequiredInstanceExtensions(&ext_count);
  for (uint32_t i = 0; i < ext_count; i++) {
    ext.emplace_back(glfw_ext[i]);
  }
}

static bool checkDeviceExtensions(const vector<const char *> &req,
                                  PhysicalDevice dev) {
  vector<ExtensionProperties> available =
      dev.enumerateDeviceExtensionProperties();
  unordered_set<string> required(req.begin(), req.end());
  for (const auto &e : available) required.erase(e.extensionName);
  return required.empty();
}

static bool checkSwapchain(PhysicalDevice dev, SurfaceKHR surface) {
  SwapchainSupportDetails details(dev, surface);
  return !details.formats.empty() && !details.presentModes.empty();
}

static void successOrThrow(Result res, const char *msg) {
  if (res != Result::eSuccess) throw runtime_error(msg);
}

DebugMessenger::DebugMessenger(Instance instance) : instance{instance} {}

void DebugMessenger::initialize() {
  DebugUtilsMessengerCreateInfoEXT ci{};
  populateDebugUtilsMessengerCreateInfo(ci);
  auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    func(static_cast<VkInstance>(instance),
         reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT *>(&ci),
         nullptr,
         reinterpret_cast<VkDebugUtilsMessengerEXT *>(&debugMessenger));
  } else {
    throw runtime_error("failed to set up debug messenger!");
  }
}

void DebugMessenger::populateDebugUtilsMessengerCreateInfo(
    DebugUtilsMessengerCreateInfoEXT &ci) {
  using Severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
  using Type = vk::DebugUtilsMessageTypeFlagBitsEXT;

  DebugUtilsMessageSeverityFlagsEXT severityFlags(
      Severity::eVerbose | Severity::eWarning | Severity::eError);
  DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
      Type::eGeneral | Type::ePerformance | Type::eValidation);
  ci.messageSeverity = severityFlags;
  ci.messageType = messageTypeFlags;
  ci.pfnUserCallback = debugCallback;
}

VkBool32 DebugMessenger::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {
  switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      log::info("Validation layer: {}", pCallbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      log::warning("Validation layer: {}", pCallbackData->pMessage);
      break;
    default:
      log::error("Validation layer: {}", pCallbackData->pMessage);
  }
  return VK_FALSE;
}

void DebugMessenger::destroy() {
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
  if (func != nullptr)
    func(static_cast<VkInstance>(instance),
         static_cast<VkDebugUtilsMessengerEXT>(debugMessenger), nullptr);
}

QueueFamilyIndices::QueueFamilyIndices(PhysicalDevice dev, SurfaceKHR surface) {
  vector<QueueFamilyProperties> queueFamilies = dev.getQueueFamilyProperties();

  int i = 0;
  for (const auto &familyProperties : queueFamilies) {
    if (familyProperties.queueFlags & QueueFlagBits::eGraphics)
      graphicsFamily = i;
    if (dev.getSurfaceSupportKHR(i, surface)) presentFamily = i;
    if (isComplete()) break;
    ++i;
  }
}

SwapchainSupportDetails::SwapchainSupportDetails(PhysicalDevice dev,
                                                 SurfaceKHR surface) {
  capabilities = dev.getSurfaceCapabilitiesKHR(surface);
  formats = dev.getSurfaceFormatsKHR(surface);
  presentModes = dev.getSurfacePresentModesKHR(surface);
}

SurfaceFormatKHR SwapchainSupportDetails::chooseFormat() {
  for (const auto &f : formats) {
    if (f.format == Format::eB8G8R8A8Srgb &&
        f.colorSpace == ColorSpaceKHR::eSrgbNonlinear)
      return f;
  }
  return formats[0];
}

Extent2D SwapchainSupportDetails::chooseSwapchainExtent(GLFWwindow *window) {
  if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    Extent2D actual = {static_cast<uint32_t>(width),
                       static_cast<uint32_t>(height)};
    actual.width = clamp(actual.width, capabilities.minImageExtent.width,
                         capabilities.maxImageExtent.width);
    actual.height = clamp(actual.height, capabilities.minImageExtent.height,
                          capabilities.maxImageExtent.height);
    return actual;
  }
}

VulkanInternals::VulkanInternals(Application &app) : app{app} {
  try {
    instance = createInstance();
    if (enableValidationLayers) {
      debugMessenger = DebugMessenger(instance);
      debugMessenger.initialize();
    }
    surface = createSurface();
    physicalDevice = pickPhysicalDevice();
    device = createLogicalDeviceAndQueues(presentQueue, graphicsQueue);
    swapchain = createSwapchain();
    swapchainImages = device.getSwapchainImagesKHR(swapchain);
    createImageViews();

    renderPass = createRenderPass();
    pipelineLayout = device.createPipelineLayout({});
    pipeline = createPipeline();
    createFramebuffers();

    commandPool = createCommandPool();
    createCommandBuffers();

    createSyncObjects();
  } catch (exception const &e) {
    log::error(e.what());
    log::error("Failed to create instance!");
    throw runtime_error("Failed to create instance!");
  }
}

Instance VulkanInternals::createInstance() {
  ApplicationInfo ai{};
  ai.pApplicationName = app.getAppName().c_str();
  ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  ai.pEngineName = "seng";
  ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  ai.apiVersion = VK_API_VERSION_1_0;

  if (enableValidationLayers && !supportsAllLayers(validationLayers))
    throw std::runtime_error("Validation layers requested, but not available");

  vector<const char *> extensions{
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};
  if (enableValidationLayers)  // extension always present if validation
                               // layers are present
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  addGlfwExtensions(extensions);

  InstanceCreateInfo ci;
  ci.pApplicationInfo = &ai;
  ci.flags |= InstanceCreateFlagBits::eEnumeratePortabilityKHR;
  ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  ci.ppEnabledExtensionNames = extensions.data();
  if (enableValidationLayers) {
    ci.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    ci.ppEnabledLayerNames = validationLayers.data();
    vk::DebugUtilsMessengerCreateInfoEXT dbg{};
    DebugMessenger::populateDebugUtilsMessengerCreateInfo(dbg);
    ci.pNext = &dbg;
  } else {
    ci.enabledLayerCount = 0;
    ci.pNext = nullptr;
  }

  return vk::createInstance(ci);
}

SurfaceKHR VulkanInternals::createSurface() {
  VkSurfaceKHR surf{};
  auto res =
      glfwCreateWindowSurface(static_cast<VkInstance>(instance),
                              app.getWindow()->getPointer(), nullptr, &surf);
  if (res != VK_SUCCESS)
    throw std::runtime_error("failed to create window surface!");
  return SurfaceKHR(surf);
}

PhysicalDevice VulkanInternals::pickPhysicalDevice() {
  vector<PhysicalDevice> devs = instance.enumeratePhysicalDevices();
  if (devs.empty())
    throw runtime_error("Failed to find GPUs with Vulkan support!");
  auto dev = find_if(devs.begin(), devs.end(), [this](const auto &dev) {
    QueueFamilyIndices queueFamilyIndices(dev, surface);

    bool queueFamilyComplete = queueFamilyIndices.isComplete();
    bool extensionSupported =
        checkDeviceExtensions(requiredDeviceExtensions, dev);
    bool swapchainAdequate =
        extensionSupported ? checkSwapchain(dev, surface) : false;
    return queueFamilyComplete && extensionSupported && swapchainAdequate;
  });
  if (dev == devs.end()) throw runtime_error("Failed to find a suitable GPU!");
  return *dev;
}

Device VulkanInternals::createLogicalDeviceAndQueues(Queue &presentQueue,
                                                     Queue &graphicsQueue) {
  QueueFamilyIndices indices(physicalDevice, surface);
  float queuePrio = 1.0f;

  vector<DeviceQueueCreateInfo> qcis;
  set<uint32_t> uniqueQueueFamilies = {*indices.graphicsFamily,
                                       *indices.presentFamily};
  for (auto queueFamily : uniqueQueueFamilies) {
    DeviceQueueCreateInfo qci{};
    qci.queueFamilyIndex = queueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &queuePrio;
    qcis.push_back(qci);
  }

  PhysicalDeviceFeatures features{};

  DeviceCreateInfo dci{};
  dci.pQueueCreateInfos = qcis.data();
  dci.queueCreateInfoCount = static_cast<uint32_t>(qcis.size());
  dci.enabledExtensionCount =
      static_cast<uint32_t>(requiredDeviceExtensions.size());
  dci.ppEnabledExtensionNames = requiredDeviceExtensions.data();
  dci.pEnabledFeatures = &features;

  Device d = physicalDevice.createDevice(dci);
  presentQueue = d.getQueue(*(indices.presentFamily), 0);
  graphicsQueue = d.getQueue(*(indices.graphicsFamily), 0);

  return d;
}

SwapchainKHR VulkanInternals::createSwapchain() {
  SwapchainSupportDetails details(physicalDevice, surface);

  SurfaceFormatKHR format = details.chooseFormat();
  swapchainFormat = format.format;
  swapchainExtent =
      details.chooseSwapchainExtent(app.getWindow()->getPointer());
  PresentModeKHR presentMode = PresentModeKHR::eFifo;
  uint32_t imageCount = details.capabilities.minImageCount + 1;
  if (details.capabilities.maxImageCount > 0 &&
      imageCount > details.capabilities.maxImageCount)
    imageCount = details.capabilities.maxImageCount;

  SwapchainCreateInfoKHR sci{};
  sci.surface = surface;
  sci.minImageCount = imageCount;
  sci.imageFormat = format.format;
  sci.imageColorSpace = format.colorSpace;
  sci.imageExtent = swapchainExtent;
  sci.imageArrayLayers = 1;
  sci.imageUsage = ImageUsageFlagBits::eColorAttachment;
  sci.preTransform = details.capabilities.currentTransform;
  sci.compositeAlpha = CompositeAlphaFlagBitsKHR::eOpaque;
  sci.presentMode = presentMode;
  sci.clipped = true;
  sci.oldSwapchain = VK_NULL_HANDLE;

  QueueFamilyIndices indices(physicalDevice, surface);
  if (indices.graphicsFamily != indices.presentFamily) {
    uint32_t queueFamilyIndices[] = {*indices.graphicsFamily,
                                     *indices.presentFamily};
    sci.imageSharingMode = SharingMode::eConcurrent;
    sci.queueFamilyIndexCount = 2;
    sci.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    sci.imageSharingMode = SharingMode::eExclusive;
    sci.queueFamilyIndexCount = 0;
    sci.pQueueFamilyIndices = nullptr;
  }

  return device.createSwapchainKHR(sci);
}

void VulkanInternals::createImageViews() {
  swapchainImageViews.resize(swapchainImages.size());
  for (size_t i = 0; i < swapchainImageViews.size(); i++) {
    ImageViewCreateInfo ci;
    ci.image = swapchainImages[i];
    ci.viewType = ImageViewType::e2D;
    ci.format = swapchainFormat;
    ci.components.r = ComponentSwizzle::eIdentity;
    ci.components.g = ComponentSwizzle::eIdentity;
    ci.components.b = ComponentSwizzle::eIdentity;
    ci.components.a = ComponentSwizzle::eIdentity;
    ci.subresourceRange.aspectMask = ImageAspectFlagBits::eColor;
    ci.subresourceRange.baseMipLevel = 0;
    ci.subresourceRange.levelCount = 1;
    ci.subresourceRange.baseArrayLayer = 0;
    ci.subresourceRange.layerCount = 1;

    swapchainImageViews[i] = device.createImageView(ci);
  }
}

RenderPass VulkanInternals::createRenderPass() {
  AttachmentDescription colorAttachment{
      {},
      swapchainFormat,
      SampleCountFlagBits::e1,
      AttachmentLoadOp::eClear,
      AttachmentStoreOp::eStore,
      AttachmentLoadOp::eDontCare,
      AttachmentStoreOp::eDontCare,
      ImageLayout::eUndefined,
      ImageLayout::ePresentSrcKHR,
  };
  AttachmentReference colorAttachmentRef{0,
                                         ImageLayout::eColorAttachmentOptimal};
  SubpassDescription subpass{};
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  SubpassDependency dep{VK_SUBPASS_EXTERNAL, 0};
  dep.srcStageMask = PipelineStageFlagBits::eColorAttachmentOutput;
  dep.srcAccessMask = AccessFlagBits::eNone;
  dep.dstStageMask = PipelineStageFlagBits::eColorAttachmentOutput;
  dep.dstAccessMask = AccessFlagBits::eColorAttachmentWrite;

  return device.createRenderPass(
      {{}, 1, &colorAttachment, 1, &subpass, 1, &dep});
}

// FIXME:: everything is stubbed out to draw only a hardcoded triangle
Pipeline VulkanInternals::createPipeline() {
  loadShadersFromDisk();

  ShaderModule vert = loadedShaders["shader.vert.spv"];
  ShaderModule frag = loadedShaders["shader.frag.spv"];

  PipelineShaderStageCreateInfo stages[] = {
      {{}, ShaderStageFlagBits::eVertex, vert, "main"},
      {{}, ShaderStageFlagBits::eFragment, frag, "main"}};

  DynamicState dynamicStates[] = {DynamicState::eViewport,
                                  DynamicState::eScissor};
  PipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.dynamicStateCount = 2;
  dynamicState.pDynamicStates = dynamicStates;

  PipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = nullptr;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr;

  PipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.topology = PrimitiveTopology::eTriangleList;
  inputAssembly.primitiveRestartEnable = false;

  PipelineViewportStateCreateInfo viewportState{};
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  PipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.depthClampEnable = false;
  rasterizer.rasterizerDiscardEnable = false;
  rasterizer.polygonMode = PolygonMode::eFill;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = CullModeFlagBits::eBack;
  rasterizer.frontFace = FrontFace::eClockwise;
  rasterizer.depthBiasEnable = false;

  PipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sampleShadingEnable = false;
  multisampling.rasterizationSamples = SampleCountFlagBits::e1;

  PipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.blendEnable = true;
  colorBlendAttachment.srcColorBlendFactor = BlendFactor::eSrcAlpha;
  colorBlendAttachment.dstColorBlendFactor = BlendFactor::eOneMinusSrcAlpha;
  colorBlendAttachment.colorBlendOp = BlendOp::eAdd;
  colorBlendAttachment.srcAlphaBlendFactor = BlendFactor::eOne;
  colorBlendAttachment.dstAlphaBlendFactor = BlendFactor::eZero;
  colorBlendAttachment.alphaBlendOp = BlendOp::eAdd;
  colorBlendAttachment.colorWriteMask =
      ColorComponentFlagBits::eR | ColorComponentFlagBits::eG |
      ColorComponentFlagBits::eB | ColorComponentFlagBits::eA;

  PipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.logicOpEnable = false;
  colorBlending.logicOp = LogicOp::eCopy;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  GraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = stages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;

  return device.createGraphicsPipelines({nullptr}, pipelineInfo).value[0];
}

void VulkanInternals::loadShadersFromDisk() {
  destroyShaders();
  for (const auto &file : filesystem::directory_iterator(app.getShaderPath())) {
    string name{file.path().filename()};
    auto buf = readFile(file.path().string());
    loadedShaders[name] = createShaderModule(buf);
  }
}

void VulkanInternals::destroyShaders() {
  if (loadedShaders.empty()) return;
  for (auto &val : loadedShaders) device.destroyShaderModule(val.second);
  loadedShaders.clear();
}

ShaderModule VulkanInternals::createShaderModule(const vector<char> &code) {
  ShaderModuleCreateInfo ci{};
  ci.codeSize = code.size();
  ci.pCode = reinterpret_cast<const uint32_t *>(code.data());
  return device.createShaderModule(ci);
}

void VulkanInternals::createFramebuffers() {
  swapchainFramebuffers.resize(swapchainImageViews.size());
  for (size_t i = 0; i < swapchainImageViews.size(); i++) {
    ImageView attachments[] = {swapchainImageViews[i]};

    FramebufferCreateInfo info{};
    info.renderPass = renderPass;
    info.attachmentCount = 1;
    info.pAttachments = attachments;
    info.width = swapchainExtent.width;
    info.height = swapchainExtent.height;
    info.layers = 1;

    swapchainFramebuffers[i] = device.createFramebuffer(info);
  }
}

CommandPool VulkanInternals::createCommandPool() {
  QueueFamilyIndices queueFamilyIndices(physicalDevice, surface);
  return device.createCommandPool(
      {CommandPoolCreateFlagBits::eResetCommandBuffer,
       queueFamilyIndices.graphicsFamily.value()});
}

void VulkanInternals::createCommandBuffers() {
  CommandBufferAllocateInfo info{};
  info.commandPool = commandPool;
  info.level = CommandBufferLevel::ePrimary;
  info.commandBufferCount = VulkanInternals::MAX_FRAMES_IN_FLIGHT;
  commandBuffers = device.allocateCommandBuffers(info);
}

void VulkanInternals::recordCommandBuffer(CommandBuffer buf,
                                          uint32_t imageIndex) {
  CommandBufferBeginInfo beginInfo{};
  successOrThrow(buf.begin(&beginInfo),
                 "Failed to start recording command buffer!");

  ClearValue clearColor{ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f}};
  RenderPassBeginInfo renderPassInfo{};
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = Offset2D{0, 0};
  renderPassInfo.renderArea.extent = swapchainExtent;
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;
  buf.beginRenderPass(renderPassInfo, SubpassContents::eInline);
  buf.bindPipeline(PipelineBindPoint::eGraphics, pipeline);
  Viewport viewport{0.0f,
                    0.0f,
                    static_cast<float>(swapchainExtent.width),
                    static_cast<float>(swapchainExtent.height),
                    0.0f,
                    1.0f};
  Rect2D scissor{{0, 0}, swapchainExtent};
  buf.setViewport(0, viewport);
  buf.setScissor(0, scissor);
  buf.draw(3, 1, 0, 0);
  buf.endRenderPass();

  buf.end();
}

void VulkanInternals::createSyncObjects() {
  imageAvailableSemaphores.resize(VulkanInternals::MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(VulkanInternals::MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(VulkanInternals::MAX_FRAMES_IN_FLIGHT);

  SemaphoreCreateInfo semInfo{};
  FenceCreateInfo fenceInfo{FenceCreateFlagBits::eSignaled};

  for (size_t i = 0; i < VulkanInternals::MAX_FRAMES_IN_FLIGHT; i++) {
    imageAvailableSemaphores[i] = device.createSemaphore(semInfo);
    renderFinishedSemaphores[i] = device.createSemaphore(semInfo);
    inFlightFences[i] = device.createFence(fenceInfo);
  }
}

void VulkanInternals::drawFrame() {
  successOrThrow(device.waitForFences(1, &inFlightFences[currentFrame], true,
                                      numeric_limits<uint64_t>::max()),
                 "Could not wait for fences");

  ResultValue<uint32_t> acquireResult =
      device.acquireNextImageKHR(swapchain, numeric_limits<uint64_t>::max(),
                                 imageAvailableSemaphores[currentFrame]);
  uint32_t imageIndex;
  switch (acquireResult.result) {
    case Result::eErrorOutOfDateKHR:
      recreateSwapchain();
      return;
    default:
      imageIndex = acquireResult.value;
  }

  successOrThrow(device.resetFences(1, &inFlightFences[currentFrame]),
                 "Could not reset fences");

  commandBuffers[currentFrame].reset();
  recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

  Semaphore waitSems[] = {imageAvailableSemaphores[currentFrame]};
  Semaphore signalSems[] = {renderFinishedSemaphores[currentFrame]};
  SwapchainKHR swapchains[] = {swapchain};

  SubmitInfo submitInfo{};
  PipelineStageFlags waitStages[] = {
      PipelineStageFlagBits::eColorAttachmentOutput};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSems;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSems;
  successOrThrow(
      graphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]),
      "Could not submit to graphics queue");

  PresentInfoKHR presentInfo{1, signalSems};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr;
  auto presentResult = presentQueue.presentKHR(presentInfo);
  if (presentResult == Result::eErrorOutOfDateKHR ||
      presentResult == Result::eSuboptimalKHR || framebufferResized) {
    framebufferResized = false;
    recreateSwapchain();
  }

  currentFrame = (currentFrame + 1) % VulkanInternals::MAX_FRAMES_IN_FLIGHT;
}

void VulkanInternals::recreateSwapchain() {
  pair<int, int> fbSize = app.getWindow()->getFramebufferSize();
  while (fbSize.first == 0 || fbSize.second == 0) {
    fbSize = app.getWindow()->getFramebufferSize();
    app.getWindow()->wait();
  }

  device.waitIdle();

  cleanupSwapchain();

  swapchain = createSwapchain();
  swapchainImages = device.getSwapchainImagesKHR(swapchain);
  createImageViews();
  createFramebuffers();
}

void VulkanInternals::cleanupSwapchain() {
  for (auto &fb : swapchainFramebuffers) device.destroyFramebuffer(fb);
  for (auto &view : swapchainImageViews) device.destroyImageView(view);
  device.destroySwapchainKHR(swapchain);
}

void VulkanInternals::signalResize() {
  framebufferResized = true;
}

VulkanInternals::~VulkanInternals() {
  device.waitIdle();

  for (size_t i = 0; i < VulkanInternals::MAX_FRAMES_IN_FLIGHT; i++) {
    device.destroySemaphore(imageAvailableSemaphores[i]);
    device.destroySemaphore(renderFinishedSemaphores[i]);
    device.destroyFence(inFlightFences[i]);
  }

  device.destroyCommandPool(commandPool);

  destroyShaders();
  device.destroyPipeline(pipeline);
  device.destroyPipelineLayout(pipelineLayout);
  device.destroyRenderPass(renderPass);
  cleanupSwapchain();
  device.destroy();

  if (enableValidationLayers) debugMessenger.destroy();
  instance.destroySurfaceKHR(surface);
  instance.destroy();
}
