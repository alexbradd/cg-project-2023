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

  m_earlyUpdateToken =
      s.onEarlyUpdate().insert(std::bind(&ScriptComponent::onEarlyUpdateImpl, this, _1));
  m_updateToken =
      s.onUpdate().insert(std::bind(&ScriptComponent::onUpdateImpl, this, _1));
  m_lateUpdateToken =
      s.onLateUpdate().insert(std::bind(&ScriptComponent::onLateUpdateImpl, this, _1));
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

  s.onEarlyUpdate().remove(m_earlyUpdateToken);
  s.onUpdate().remove(m_updateToken);
  s.onLateUpdate().remove(m_lateUpdateToken);
}
