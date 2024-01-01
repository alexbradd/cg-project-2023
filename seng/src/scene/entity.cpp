#include <seng/components/component_ptr.hpp>
#include <seng/components/transform.hpp>
#include <seng/log.hpp>
#include <seng/scene/entity.hpp>

#include <algorithm>
#include <memory>

using namespace seng;
using namespace std;

uint64_t Entity::INDEX_COUNTER = 0;
const Entity::ComponentList Entity::EMPTY_VECTOR;

Entity::Entity(Application& app, Scene& s, std::string n) :
    m_app(std::addressof(app)),
    m_scene(std::addressof(s)),
    m_id(INDEX_COUNTER++),
    m_name(std::move(n)),
    m_transform(std::make_unique<Transform>(*this))
{
  m_transform->initialize();
}

Entity::~Entity() = default;

void Entity::transform(ComponentPtr&& t)
{
  if (t != nullptr) {
    m_transform = std::move(t);
  } else {
    seng::log::warning("Passing null transform, ignoring. Something's wrong...");
  }
}

void Entity::untypedInsert(const ComponentIdType& id, ComponentPtr&& cmp)
{
  if (!checkAndWarnCompPtr(cmp)) {
    m_components[id].push_back(std::move(cmp));
    m_components[id].back()->initialize();
  }
}

bool Entity::checkAndWarnCompPtr(ComponentPtr& ptr)
{
  if (ptr == nullptr) {
    seng::log::warning("Passing null component, ignoring. Something's wrong...");
  }
  return ptr == nullptr;
}

void Entity::removeWithIterByPtr(ComponentMap::iterator it, const BaseComponent* ptr)
{
  if (it == m_components.end()) {
    seng::log::warning("Attempting to remove component type that is not attached");
    return;
  }
  auto rmEnd = std::remove_if(it->second.begin(), it->second.end(),
                              [&](auto& unique) { return unique.get() == ptr; });
  it->second.erase(rmEnd, it->second.end());
}
