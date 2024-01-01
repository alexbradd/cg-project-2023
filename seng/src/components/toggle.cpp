#include <seng/components/base_component.hpp>
#include <seng/components/definitions.hpp>
#include <seng/components/toggle.hpp>

using namespace seng;

ToggleComponent::ToggleComponent(Entity &entity, bool enabled) : BaseComponent(entity)
{
  m_enabled = enabled;
}

void ToggleComponent::enabled(bool b)
{
  m_enabled = b;
  if (m_enabled)
    onEnable();
  else
    onDisable();
}

void ToggleComponent::enable()
{
  enabled(true);
}

void ToggleComponent::disable()
{
  enabled(false);
}

void ToggleComponent::toggle()
{
  enabled(!m_enabled);
}
