#include <cstddef>
#include <iterator>
#include <seng/log.hpp>
#include <seng/primitive_types.hpp>
#include <seng/utils.hpp>
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
static vector<VulkanFramebuffer> createFramebuffers(VulkanDevice &,
                                                    VulkanSwapchain &,
                                                    VulkanRenderPass &);
// Stub geometry uploading
static void uploadTo(VulkanDevice &device,
                     CommandPool &pool,
                     Queue &queue,
                     VulkanBuffer &to,
                     vk::DeviceSize size,
                     vk::DeviceSize offset,
                     const void *data) {
  vk::MemoryPropertyFlags hostVisible =
      vk::MemoryPropertyFlagBits::eHostCoherent |
      vk::MemoryPropertyFlagBits::eHostVisible;

  VulkanBuffer temp(device, vk::BufferUsageFlagBits::eTransferSrc, size,
                    hostVisible, true);
  temp.load(data, 0, size, {});
  temp.copy(to, {0, offset, size}, pool, queue);
}

static constexpr vk::CommandPoolCreateFlags cmdPoolFlags =
    vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

static constexpr vk::BufferUsageFlags vertexBufferUsage =
    vk::BufferUsageFlagBits::eVertexBuffer |
    vk::BufferUsageFlagBits::eTransferSrc |
    vk::BufferUsageFlagBits::eTransferDst;
static constexpr vk::BufferUsageFlags indexBufferUsage =
    vk::BufferUsageFlagBits::eIndexBuffer |
    vk::BufferUsageFlagBits::eTransferSrc |
    vk::BufferUsageFlagBits::eTransferDst;

VulkanRenderer::VulkanRenderer(ApplicationConfig config, GlfwWindow &window) :
    window(window),
    context(),
    // Instance creation
    _instance(createInstance(context, window)),
    debugMessenger(_instance, VulkanRenderer::useValidationLayers),
    _surface(window.createVulkanSurface(_instance)),

    // Device, swapchain and framebuffers
    device(_instance, _surface),
    swapchain(device, _surface, window),
    renderPass(device, swapchain),
    framebuffers(createFramebuffers(device, swapchain, renderPass)),

    // Command pools and buffers
    cmdPool(device.logical(),
            {cmdPoolFlags, *device.queueFamiliyIndices().graphicsFamily()}),
    graphicsCmdBufs(
        many<VulkanCommandBuffer>(swapchain.images().size(), device, cmdPool)),

    // Sync objects
    imageAvailableSems(many<Semaphore>(swapchain.images().size(),
                                       device.logical(),
                                       vk::SemaphoreCreateInfo{})),
    queueCompleteSems(many<Semaphore>(swapchain.images().size(),
                                      device.logical(),
                                      vk::SemaphoreCreateInfo{})),
    inFlightFences(many<VulkanFence>(swapchain.images().size(), device, true)),
    imgsInFlight(swapchain.images().size()),

    // Buffers
    vertexBuffer(device, vertexBufferUsage, sizeof(Vertex) * 1024 * 1024),
    indexBuffer(device, indexBufferUsage, sizeof(Vertex) * 1024 * 1024),

    // Shaders
    shaderLoader(device, renderPass, config.shaderPath) {
  log::info("Vulkan context is up and running!");
  shaderLoader.loadShaders();

  // FIXME: Temporary geometry
  log::dbg("Loading test geometry");
  verts[0].pos = glm::vec3(0.0, 0.5, 0.0);
  verts[1].pos = glm::vec3(0.5, 0.5, 0.0);
  verts[2].pos = glm::vec3(0.0, -0.5, 0.0);
  verts[3].pos = glm::vec3(0.5, -0.5, 0.0);
  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  indices[3] = 1;
  indices[4] = 3;
  indices[5] = 2;
  uploadTo(device, cmdPool, device.graphicsQueue(), vertexBuffer,
           sizeof(Vertex) * 4, 0, verts);
  uploadTo(device, cmdPool, device.graphicsQueue(), indexBuffer,
           sizeof(uint32_t) * 6, 0, indices);
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

vector<VulkanFramebuffer> createFramebuffers(VulkanDevice &dev,
                                             VulkanSwapchain &swap,
                                             VulkanRenderPass &pass) {
  vector<VulkanFramebuffer> fbs{};
  fbs.reserve(swap.images().size());

  for (auto &img : swap.images()) {
    vector<vk::ImageView> attachments(2);  // TODO: make configurable
    attachments[0] = *img;
    attachments[1] = **swap.depthBuffer().imageView();
    fbs.emplace_back(dev, pass, swap.extent(), attachments);
  }
  return fbs;
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

    // Begin the render pass
    renderPass.begin(curBuf, curFb);

    // FIXME: temporary shader
    shaderLoader.getShader("default")->use(curBuf);
    //
    // FIXME: temporary geometry
    curBuf.buffer().bindVertexBuffers(0, {*vertexBuffer.buffer()}, {0});
    curBuf.buffer().bindIndexBuffer(*indexBuffer.buffer(), 0,
                                    vk::IndexType::eUint32);

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

    curBuf.buffer().drawIndexed(6, 1, 0, 0, 0);

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
