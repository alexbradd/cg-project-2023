#include <seng/application.hpp>
#include <seng/components/base_component.hpp>
#include <seng/components/script.hpp>
#include <seng/scene/scene.hpp>

#include <functional>

using namespace seng;
using namespace seng::components;

using namespace std::placeholders;

ScriptComponent::ScriptComponent(scene::Entity &entity) : BaseComponent(entity) {}

void ScriptComponent::initialize()
{
  auto &s = entity->scene();

  m_earlyUpdateToken = s.listen(scene::SceneEvents::EARLY_UPDATE,
                                std::bind(&ScriptComponent::onEarlyUpdate, this, _1));
  m_updateToken = s.listen(scene::SceneEvents::UPDATE,
                           std::bind(&ScriptComponent::onUpdate, this, _1));
  m_lateUpdateToken = s.listen(scene::SceneEvents::UPDATE,
                               std::bind(&ScriptComponent::onLateUpdate, this, _1));
  scriptInitialize();
}

ScriptComponent::~ScriptComponent()
{
  auto &s = entity->scene();

  s.unlisten(m_earlyUpdateToken);
  s.unlisten(m_updateToken);
  s.unlisten(m_lateUpdateToken);
}
