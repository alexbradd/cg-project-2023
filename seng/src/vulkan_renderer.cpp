#include <cstddef>
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
      imgsInFlight(swapchain.images().size()) {
  log::info("Vulkan context is up and running!");
}

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

void VulkanRenderer::signalResize() { fbGeneration++; }

void VulkanRenderer::beginFrame() {
  if (recreatingSwapchain) {
    device.logical().waitIdle();
    throw BeginFrameException("Already recreating swapchain, waiting...");
  }

  if (lastFbGeneration != fbGeneration) {
    device.logical().waitIdle();
    recreateSwapchain();
    throw BeginFrameException("Frambuffer changed, aborting...");
  }

  try {
    inFlightFences[currentFrame].wait();
    imageIndex = swapchain.nextImageIndex(imageAvailableSems[currentFrame]);

    VulkanFramebuffer &curFb = framebuffers[imageIndex];
    VulkanCommandBuffer &curBuf = graphicsCmdBufs[imageIndex];
    curBuf.reset();
    curBuf.begin();

    // Dynamic states
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain.extent().width);
    viewport.height = static_cast<float>(swapchain.extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    curBuf.buffer().setViewport(0, viewport);

    vk::Rect2D scissor{{0, 0}, swapchain.extent()};
    curBuf.buffer().setScissor(0, scissor);

    // Begin the render pass
    renderPass.begin(curBuf, curFb);

  } catch (const exception &e) {
    log::warning("Caught exception: {}", e.what());
    throw BeginFrameException("Caught exception while starting recording...");
  }
}

void VulkanRenderer::endFrame() {
  VulkanCommandBuffer &curBuf = graphicsCmdBufs[imageIndex];

  renderPass.end(curBuf);
  curBuf.end();

  // Make sure the previous frame is not using this image
  if (imgsInFlight[imageIndex] != nullptr) imgsInFlight[imageIndex]->wait();
  // Then mark the image fence as in-use by this frame
  imgsInFlight[imageIndex] = &inFlightFences.data()[currentFrame];
  // Reset the fence for use on the next frame
  inFlightFences[currentFrame].reset();

  vk::SubmitInfo submit_info{};
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &(*curBuf.buffer());

  // The semaphore(s) to be signaled when the queue is complete.
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &(*queueCompleteSems[currentFrame]);

  // Wait semaphore ensures that the operation cannot begin until the image is
  // available
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &(*imageAvailableSems[currentFrame]);

  // Each semaphore waits on the corresponding pipeline stage to complete.
  // 1:1 ratio. VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT prevents
  // subsequent colour attachment writes from executing until the semaphore
  // signals (i.e. one frame is presented at a time)
  vk::PipelineStageFlags flags[1] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};
  submit_info.pWaitDstStageMask = flags;

  device.graphicsQueue().submit(submit_info,
                                *inFlightFences[currentFrame].handle());
  curBuf.setSubmitted();

  try {
    swapchain.present(device.presentQueue(), device.graphicsQueue(),
                      queueCompleteSems[currentFrame], imageIndex);
  } catch (const InadequateSwapchainException &e) {
    log::info("Error while presenting: swapchain out of date. Recreating...");
    recreateSwapchain();
  }

  currentFrame = (currentFrame + 1) % swapchain.MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::recreateSwapchain() {
  // If already recreating, do nothing.
  if (recreatingSwapchain) return;

  // Get the new framebuffer size, if null do nothing
  pair<unsigned int, unsigned int> fbSize = window.get().framebufferSize();
  if (fbSize.first == 0 || fbSize.second == 0) {
    log::dbg("Null framebuffer, aborting...");
    return;
  }

  // Start the creation process
  recreatingSwapchain = true;
  log::info("Started swapchain recreation");

  // Wait for any pending work to finish and flush out temporary data
  device.logical().waitIdle();
  for (size_t i = 0; i < imgsInFlight.size(); i++) imgsInFlight[i] = nullptr;

  // Requery swapchain support data (it might have changed)
  device.requerySupport();
  device.requeryDepthFormat();

  // Recreate the swapchain and clear the currentFrame counter
  VulkanSwapchain::recreate(swapchain, device, _surface, window);
  currentFrame = 0;

  // Sync framebuffer generation
  lastFbGeneration = fbGeneration;

  // Clear old command buffers and frabuffers
  graphicsCmdBufs.clear();
  framebuffers.clear();

  // Update the render pass dimesions
  renderPass.updateOffset({0, 0});
  renderPass.updateExtent(swapchain.extent());

  // Create new framebuffers and command buffers
  framebuffers = createFramebuffers(device, swapchain, renderPass);
  graphicsCmdBufs = VulkanCommandBuffer::createMultiple(
      device, cmdPool, swapchain.images().size());

  // Finish the recreation process
  recreatingSwapchain = false;
  log::info("Finished swapchain recreation");
}

void VulkanRenderer::draw() {
  try {
    beginFrame();
    endFrame();
  } catch (const BeginFrameException &e) {
    log::info("Could not begin frame: {}", e.what());
  } catch (const exception &e) {
    log::warning("Unhandled exception reached draw function: {}", e.what());
  }
}

VulkanRenderer::~VulkanRenderer() {
  // Just checking if the instance handle is valid is enough
  // since all objects are valid or none are.
  if (*_instance != vk::Instance{}) {
    device.logical().waitIdle();
    log::info("Destroying vulkan context");
  }
}
