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
#include <seng/yaml_utils.hpp>

#include <yaml-cpp/yaml.h>
#include <vulkan/vulkan_raii.hpp>

#include <filesystem>
#include <memory>
#include <string>

using namespace seng;
using namespace seng::rendering;
using namespace std;

// Misc functions
static bool isYamlNodeValidShader(const YAML::Node &node);

Scene::Scene(Application &app) :
    m_app(std::addressof(app)), m_renderer(app.renderer().get()), m_mainCamera(nullptr)
{
  seng::log::dbg("Created new scene");
}

std::unique_ptr<Scene> Scene::loadFromDisk(Application &app, std::string sceneName)
{
  std::unique_ptr<Scene> s = std::make_unique<Scene>(app);
  auto &config = s->m_app->config();

  // Parse config file
  filesystem::path scene{filesystem::path{config.scenePath} /
                         filesystem::path{sceneName + ".yml"}};

  YAML::Node sceneConfig;

  try {
    sceneConfig = YAML::LoadFile(scene.string());
  } catch (exception &e) {
    seng::log::error("Unable to load scene: {}", e.what());
    return nullptr;
  }

  if (sceneConfig["Shaders"] && sceneConfig["Shaders"].IsSequence()) {
    auto shaders = sceneConfig["Shaders"];
    for (YAML::const_iterator i = shaders.begin(); i != shaders.end(); ++i) {
      s->parseShader(config.shaderPath, *i);
    }
  }

  // TODO: Shaders instances
  if (sceneConfig["ShaderInstances"]) seng::log::dbg("Got shader instances");

  // Parse mesh list
  if (sceneConfig["Meshes"] && sceneConfig["Meshes"].IsSequence()) {
    auto meshes = sceneConfig["Meshes"];
    for (YAML::const_iterator i = meshes.begin(); i != meshes.end(); i++)
      s->parseMesh(*i);
  }

  // Parse light
  if (sceneConfig["Light"] && sceneConfig["Light"].IsMap()) {
    auto light = sceneConfig["Light"];
    if (light["ambient"] && light["ambient"].IsSequence())
      s->m_ambient = light["ambient"].as<glm::vec4>();
    if (light["color"] && light["color"].IsSequence())
      s->m_directLight.color(light["color"].as<glm::vec4>());
    if (light["direction"] && light["direction"].IsSequence())
      s->m_directLight.direction(light["direction"].as<glm::vec3>());
  }

  // Load entities
  if (sceneConfig["Entities"] && sceneConfig["Entities"].IsSequence()) {
    auto e = sceneConfig["Entities"];
    for (YAML::const_iterator i = e.begin(); i != e.end(); ++i) s->parseEntity(*i);
  }

  return s;
}

void Scene::parseShader(const string &shaderPath, const YAML::Node &shader)
{
  if (!isYamlNodeValidShader(shader)) {
    seng::log::warning("Malformed YAML shader, skipping");
    return;
  }

  std::string vertName = shader["vert"].as<string>();
  m_stages.try_emplace(vertName, m_renderer->device(), shaderPath, vertName,
                       ShaderStage::Type::eVertex);

  std::string fragName = shader["frag"].as<string>();
  m_stages.try_emplace(fragName, m_renderer->device(), shaderPath, fragName,
                       ShaderStage::Type::eFragment);

  std::string shaderName = shader["name"].as<string>();
  vector<const ShaderStage *> s{&m_stages.at(vertName), &m_stages.at(fragName)};
  m_shaders.try_emplace(shaderName, *m_renderer, m_renderer->globalUniform(), shaderName,
                        s);
}

bool isYamlNodeValidShader(const YAML::Node &node)
{
  return node.IsMap() && node["name"] && node["name"].IsScalar() && node["vert"] &&
         node["vert"].IsScalar() && node["frag"] && node["frag"].IsScalar();
}

void Scene::parseMesh(const YAML::Node &node)
{
  std::string name = node.as<string>();
  auto &m = m_renderer->requestMesh(name);
  m.sync();
}

void Scene::parseEntity(const YAML::Node &node)
{
  if (!node.IsMap()) {
    seng::log::warning("Malformed YAML node");
    return;
  }

  std::string name = "Entity";
  if (node["name"] && node["name"].IsScalar()) name = node["name"].as<string>();
  Entity *ret = newEntity(name);

  if (node["transform"] && node["transform"].IsMap()) {
    auto &t = node["transform"];
    auto ptr = SceneConfigComponentFactory::create(*ret, Transform::componentId(), t);
    ret->transform(std::move(ptr));
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
      auto ptr = SceneConfigComponentFactory::create(*ret, id, comp);
      ret->untypedInsert(id, std::move(ptr));
    }
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

void Scene::mainCamera(Camera *cam)
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

  if (m_mainCamera == nullptr) return;

  m_renderer->globalUniform().projection().projection = m_mainCamera->projectionMatrix();
  m_renderer->globalUniform().projection().view = m_mainCamera->viewMatrix();

  m_renderer->globalUniform().lighting().ambientColor = m_ambient;
  m_renderer->globalUniform().lighting().lightColor = m_directLight.color();
  m_renderer->globalUniform().lighting().lightDir = m_directLight.direction();
  m_renderer->globalUniform().lighting().cameraPosition =
      m_mainCamera->attachedTo().transform()->position();

  m_renderer->globalUniform().update(handle);

  for (auto &shaderNamePair : m_shaders) {
    auto &shader = shaderNamePair.second;

    shader.bindDescriptorSets(handle, cmd);
    shader.use(cmd);

    // FIXME: should be per entity rendererd with the shader not per mesh
    for (const auto &mesh : m_renderer->meshes()) {
      if (!mesh.second.synced()) continue;

      shader.updateModelState(cmd, glm::mat4(1));
      cmd.buffer().bindVertexBuffers(0, *(*mesh.second.vertexBuffer()).buffer(), {0});
      cmd.buffer().bindIndexBuffer(*(*mesh.second.indexBuffer()).buffer(), 0,
                                   vk::IndexType::eUint32);
      cmd.buffer().drawIndexed(mesh.second.indices().size(), 1, 0, 0, 0);
    }
  }
}

void Scene::update(Timestamp lastFrame, const FrameHandle &handle)
{
  float deltaTime = inSeconds(Clock::now() - lastFrame);
  m_earlyUpdate(deltaTime);

  // Update
  m_update(deltaTime);
  draw(handle);

  // Late update
  m_lateUpdate(deltaTime);
}

Scene::~Scene()
{
  // Ensure that every operation relative to this scene has been completed
  m_renderer->device().logical().waitIdle();
  seng::log::dbg("Deallocated scene");
}
