#include <seng/application.hpp>
#include <seng/components/camera.hpp>
#include <seng/entity.hpp>
#include <seng/log.hpp>
#include <seng/rendering/object_shader.hpp>
#include <seng/rendering/primitive_types.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/rendering/shader_stage.hpp>
#include <seng/scene/scene.hpp>

#include <yaml-cpp/yaml.h>
#include <vulkan/vulkan_raii.hpp>

#include <filesystem>
#include <memory>
#include <unordered_set>

using namespace seng::scene;
using namespace seng::rendering;
using namespace std;

#define VERT_NAME "simple_vert"
#define FRAG_NAME "simple_frag"
#define SHADER_NAME "default"

// Initializer functions
static vk::raii::DescriptorSetLayout createGlobalDescriptorLayout(const Device &device);

Scene::Scene(Application &app) :
    app(std::addressof(app)),
    renderer(app.renderer().get()),
    globalDescriptorSetLayout(createGlobalDescriptorLayout(renderer->getDevice())),
    mainCamera(nullptr)
{
  renderer->requestDescriptorSet(*globalDescriptorSetLayout);
}

vk::raii::DescriptorSetLayout createGlobalDescriptorLayout(const Device &device)
{
  vk::DescriptorSetLayoutBinding guboBinding{};
  guboBinding.binding = 0;
  guboBinding.descriptorCount = 1;
  guboBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
  guboBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

  vk::DescriptorSetLayoutCreateInfo info{};
  info.bindingCount = 1;
  info.pBindings = &guboBinding;
  return vk::raii::DescriptorSetLayout(device.logical(), info);
}

void Scene::loadFromDisk(std::string sceneName)
{
  // clear old state
  stages.clear();
  shaders.clear();
  // TODO: clear old instances
  meshes.clear();
  sceneGraph.clear();
  mainCamera = nullptr;

  auto &config = app->config();
  // Parse config file
  filesystem::path scene{filesystem::path{config.scenePath} /
                         filesystem::path{sceneName + ".yml"}};
  YAML::Node sceneConfig = YAML::LoadFile(scene.string());
  /* unordered_set<string> shaderStageNames; */
  /* unordered_set<string> shaderNames; */
  // TODO: how to go about instances?
  unordered_set<string> meshNames;

  // TODO: Shaders
  if (sceneConfig["Shaders"]) seng::log::dbg("Got shaders");

  // TODO: Shaders isntances
  if (sceneConfig["ShaderInstances"]) seng::log::dbg("Got shader instances");

  // Parse mesh list
  if (sceneConfig["Meshes"]) {
    auto meshes = sceneConfig["Meshes"];
    for (YAML::const_iterator i = meshes.begin(); i != meshes.end(); i++) {
      meshNames.insert(i->as<string>());
    }
  }

  // TODO: parse entities
  if (sceneConfig["Entities"]) seng::log::dbg("Got entities");

  // Load ShaderStages
  // FIXME: stub
  stages.try_emplace(VERT_NAME, renderer->getDevice(), config.shaderPath, VERT_NAME,
                     ShaderStage::Type::eVertex);
  stages.try_emplace(FRAG_NAME, renderer->getDevice(), config.shaderPath, FRAG_NAME,
                     ShaderStage::Type::eFragment);

  // Load ObjectShaders
  // FIXME: stub
  vector<const ShaderStage *> s{&stages.at(VERT_NAME), &stages.at(FRAG_NAME)};
  shaders.try_emplace(SHADER_NAME, renderer->getDevice(), renderer->getRenderPass(),
                      globalDescriptorSetLayout, SHADER_NAME, s);

  // TODO: load shader instances

  // Load meshes
  for (const auto &s : meshNames) {
    meshes.emplace(s, Mesh::loadFromDisk(*renderer, config, s));
  }

  // TODO: entities
  auto e = sceneGraph.newEntity("cam");  // FIXME: stub
  e->emplaceComponent<components::Camera>(*app, *e);
  e->getTransform()->translate(0, 0, 10);
}

void Scene::registerCamera(seng::components::Camera *cam)
{
  if (cam == nullptr) {
    seng::log::warning("Trying to register null camera");
    return;
  }
  mainCamera = cam;
}

void Scene::draw(const FrameHandle &handle)
{
  const auto &cmd = renderer->getCommandBuffer(handle);
  auto &shader = shaders.at(SHADER_NAME);  // FIXME: stub

  if (mainCamera == nullptr) return;

  shader.globalUniformObject().projection = mainCamera->projectionMatrix();
  shader.globalUniformObject().view = mainCamera->viewMatrix();
  shader.uploadGlobalState(
      cmd, renderer->getDescriptorSet(handle, *globalDescriptorSetLayout));

  for (const auto &mesh : meshes) {  // FIXME: should be per game object not per mesh
    shader.updateModelState(cmd, glm::mat4(1));
    cmd.buffer().bindVertexBuffers(0, *mesh.second.vertexBuffer().buffer(), {0});
    cmd.buffer().bindIndexBuffer(*mesh.second.indexBuffer().buffer(), 0,
                                 vk::IndexType::eUint32);
    shaders.at(SHADER_NAME).use(cmd);  // FIXME: stub
    cmd.buffer().drawIndexed(mesh.second.indexCount(), 1, 0, 0, 0);
  }
}

Scene::~Scene()
{
  // Clear descriptors only if we are not in a moved-from state
  if (*globalDescriptorSetLayout != vk::DescriptorSetLayout{}) {
    // Ensure that every operation relative to this scene has been completed
    renderer->getDevice().logical().waitIdle();

    // Clear requested resources
    renderer->clearDescriptorSets();
  }
}
