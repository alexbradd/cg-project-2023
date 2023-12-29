#include <seng/application.hpp>
#include <seng/components/camera.hpp>
#include <seng/components/transform.hpp>
#include <seng/log.hpp>
#include <seng/rendering/object_shader.hpp>
#include <seng/rendering/primitive_types.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/rendering/shader_stage.hpp>
#include <seng/scene/entity.hpp>
#include <seng/scene/scene.hpp>

#include <yaml-cpp/yaml.h>
#include <tuple>
#include <vulkan/vulkan_raii.hpp>

#include <filesystem>
#include <functional>
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

uint64_t Scene::EVENT_INDEX = 0;

Scene::Scene(Application &app) :
    m_app(std::addressof(app)),
    m_renderer(app.renderer().get()),
    m_globalDescriptorSetLayout(createGlobalDescriptorLayout(m_renderer->device())),
    m_mainCamera(nullptr)
{
  m_renderer->requestDescriptorSet(*m_globalDescriptorSetLayout);
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
  m_stages.clear();
  m_shaders.clear();
  // TODO: clear old instances
  m_meshes.clear();
  removeAllEntities();
  m_mainCamera = nullptr;

  auto &config = m_app->config();
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

  // Load ShaderStages
  // FIXME: stub
  m_stages.try_emplace(VERT_NAME, m_renderer->device(), config.shaderPath, VERT_NAME,
                       ShaderStage::Type::eVertex);
  m_stages.try_emplace(FRAG_NAME, m_renderer->device(), config.shaderPath, FRAG_NAME,
                       ShaderStage::Type::eFragment);

  // Load ObjectShaders
  // FIXME: stub
  vector<const ShaderStage *> s{&m_stages.at(VERT_NAME), &m_stages.at(FRAG_NAME)};
  m_shaders.try_emplace(SHADER_NAME, m_renderer->device(), m_renderer->renderPass(),
                        m_globalDescriptorSetLayout, SHADER_NAME, s);

  // TODO: load shader instances

  // Load meshes
  for (const auto &s : meshNames) {
    m_meshes.emplace(s, Mesh::loadFromDisk(*m_renderer, config, s));
  }

  // Load entities
  if (sceneConfig["Entities"] && sceneConfig["Entities"].IsSequence()) {
    auto e = sceneConfig["Entities"];
    for (YAML::const_iterator i = e.begin(); i != e.end(); ++i) newEntity(*i);
  }
}

Scene::EntityList::const_iterator Scene::findByName(const std::string &name) const
{
  return std::find_if(m_entities.begin(), m_entities.end(),
                      [&](const auto &elem) { return elem.name() == name; });
}

Scene::EntityList::iterator Scene::findByName(const std::string &name)
{
  return std::find_if(m_entities.begin(), m_entities.end(),
                      [&](const auto &elem) { return elem.name() == name; });
}

std::vector<const Entity *> Scene::findAllByName(const std::string &name) const
{
  vector<const Entity *> ptrs;
  for (const auto &e : m_entities) {
    if (e.name() == name) ptrs.push_back(std::addressof(e));
  }
  return ptrs;
}

std::vector<Entity *> Scene::findAllByName(const std::string &name)
{
  vector<Entity *> ptrs;
  for (auto &e : m_entities) {
    if (e.name() == name) ptrs.push_back(std::addressof(e));
  }
  return ptrs;
}

Entity *Scene::newEntity(std::string name)
{
  m_entities.push_back(Entity(*m_app, *this, name));
  return &m_entities.back();
}

Entity *Scene::newEntity(const YAML::Node &node)
{
  using namespace seng::components;

  if (!node.IsMap()) {
    seng::log::warning("Malformed YAML node");
    return nullptr;
  }

  std::string name = "Entity";
  if (node["name"] && node["name"].IsScalar()) name = node["name"].as<string>();
  Entity *ret = newEntity(name);

  if (node["transform"] && node["transform"].IsMap()) {
    auto &t = node["transform"];
    auto ptr = SceneConfigComponentFactory::create(*ret, Transform::componentId(), t);
    auto concrete = concreteUniquePtr<Transform>(std::move(ptr));
    ret->transform(std::move(concrete));
  }

  if (node["components"] && node["components"].IsSequence()) {
    auto &comps = node["components"];
    for (YAML::const_iterator i = comps.begin(); i != comps.end(); ++i) {
      auto &comp = *i;
      if (!comp.IsMap() || !comp["id"] || !comp["id"].IsScalar()) {
        seng::log::warning("Malformed YAML component, skipping...");
        continue;
      }
      std::string id = comp["id"].as<string>();
      std::unique_ptr<BaseComponent> ptr =
          SceneConfigComponentFactory::create(*ret, id, comp);
      ret->untypedInsert(id, std::move(ptr));
    }
  }
  return ret;
}

void Scene::removeEntity(EntityList::const_iterator i)
{
  m_entities.erase(i);
}

void Scene::removeEntity(const Entity *e)
{
  if (e == nullptr) {
    seng::log::warning("Tried to remove a null entity... Something is wrong");
    return;
  }
  auto it = std::find(m_entities.begin(), m_entities.end(), *e);
  if (it == m_entities.end()) {
    seng::log::warning(
        "Tried to remove an entity not registered in the scene graph... Something is "
        "wrong");
    return;
  }
  this->removeEntity(it);
}

void Scene::removeEntity(const string &name)
{
  auto it = findByName(name);
  if (it == m_entities.end()) {
    seng::log::warning("Could not find an entity with the given name to remove");
    return;
  }
  this->removeEntity(it);
}

void Scene::removeAllEntities()
{
  m_entities.clear();
}

void Scene::mainCamera(components::Camera *cam)
{
  if (cam == nullptr) {
    seng::log::warning("Trying to register null camera");
    return;
  }
  m_mainCamera = cam;
}

void Scene::draw(const FrameHandle &handle)
{
  const auto &cmd = m_renderer->getCommandBuffer(handle);
  auto &shader = m_shaders.at(SHADER_NAME);  // FIXME: stub

  if (m_mainCamera == nullptr) return;

  shader.globalUniformObject().projection = m_mainCamera->projectionMatrix();
  shader.globalUniformObject().view = m_mainCamera->viewMatrix();
  shader.uploadGlobalState(
      cmd, m_renderer->getDescriptorSet(handle, *m_globalDescriptorSetLayout));

  for (const auto &mesh : m_meshes) {  // FIXME: should be per game object not per mesh
    shader.updateModelState(cmd, glm::mat4(1));
    cmd.buffer().bindVertexBuffers(0, *mesh.second.vertexBuffer().buffer(), {0});
    cmd.buffer().bindIndexBuffer(*mesh.second.indexBuffer().buffer(), 0,
                                 vk::IndexType::eUint32);
    m_shaders.at(SHADER_NAME).use(cmd);  // FIXME: stub
    cmd.buffer().drawIndexed(mesh.second.indexCount(), 1, 0, 0, 0);
  }
}

SceneEventToken Scene::listen(SceneEvents e, std::function<void(float)> h)
{
  EventHandlerType handler = make_tuple(EVENT_INDEX++, h);
  m_eventHandlers[e].push_back(handler);
  SceneEventToken tok;
  tok.event = e;
  tok.id = EVENT_INDEX;
  return tok;
}

void Scene::unlisten(SceneEventToken tok)
{
  auto i = m_eventHandlers.find(tok.event);
  if (i == m_eventHandlers.end()) return;

  auto end = std::remove_if(i->second.begin(), i->second.end(),
                            [&](const auto &h) { return tok.id == std::get<0>(h); });
  i->second.erase(end, i->second.end());
}

void Scene::fireEventType(SceneEvents event, float delta) const
{
  auto i = m_eventHandlers.find(event);
  if (i == m_eventHandlers.end()) return;
  for (const auto &ev : i->second) std::get<1>(ev)(delta);
}

Scene::~Scene()
{
  // Clear descriptors only if we are not in a moved-from state
  if (*m_globalDescriptorSetLayout != vk::DescriptorSetLayout{}) {
    // Ensure that every operation relative to this scene has been completed
    m_renderer->device().logical().waitIdle();

    // Clear requested resources
    m_renderer->clearDescriptorSets();
  }
}
