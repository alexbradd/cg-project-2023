#include <seng/application.hpp>
#include <seng/components/base_component.hpp>
#include <seng/components/script.hpp>
#include <seng/scene/scene.hpp>

#include <functional>

using namespace seng;

using namespace std::placeholders;

ScriptComponent::ScriptComponent(Entity &e, bool enabled) : ToggleComponent(e, enabled)
{
  auto &s = entity->scene();

  m_earlyUpdateToken = s.listen(SceneEvents::EARLY_UPDATE,
                                std::bind(&ScriptComponent::onEarlyUpdateImpl, this, _1));
  m_updateToken =
      s.listen(SceneEvents::UPDATE, std::bind(&ScriptComponent::onUpdateImpl, this, _1));
  m_lateUpdateToken = s.listen(SceneEvents::UPDATE,
                               std::bind(&ScriptComponent::onLateUpdateImpl, this, _1));
}

// Not very efficient implementation but it works.
// TODO: make it better by using the onEnable/onDisable to dynamically register/unregister
void ScriptComponent::onEarlyUpdateImpl(float deltaTime)
{
  if (enabled()) onEarlyUpdate(deltaTime);
}

void ScriptComponent::onUpdateImpl(float deltaTime)
{
  if (enabled()) onUpdate(deltaTime);
}

void ScriptComponent::onLateUpdateImpl(float deltaTime)
{
  if (enabled()) onLateUpdate(deltaTime);
}

ScriptComponent::~ScriptComponent()
{
  auto &s = entity->scene();

  s.unlisten(m_earlyUpdateToken);
  s.unlisten(m_updateToken);
  s.unlisten(m_lateUpdateToken);
}
