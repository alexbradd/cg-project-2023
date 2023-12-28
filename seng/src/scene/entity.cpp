#include <seng/components/transform.hpp>
#include <seng/log.hpp>
#include <seng/scene/entity.hpp>

#include <memory>

using namespace seng::scene;
using namespace seng::components;
using namespace std;

uint64_t Entity::INDEX_COUNTER = 0;

Entity::Entity(Application& app, std::string n) :
    id(INDEX_COUNTER++),
    name(std::move(n)),
    transform(std::make_unique<components::Transform>(app, *this))
{
  transform->initialize();
}

void Entity::setTransform(std::unique_ptr<components::Transform>&& t)
{
  if (t != nullptr) {
    transform = std::move(t);
  } else {
    seng::log::warning("Passing null transform, ignoring. Something's wrong...");
  }
}

bool Entity::checkAndWarnCompPtr(std::unique_ptr<components::BaseComponent>& ptr)
{
  if (ptr == nullptr) {
    seng::log::warning("Passing null component, ignoring. Something's wrong...");
  }
  return ptr == nullptr;
}

void Entity::removeWithIterByPtr(ComponentMap::iterator it,
                                 const components::BaseComponent* ptr)
{
  if (it == components.end()) {
    seng::log::warning("Attempting to remove component type that is not attached");
    return;
  }
  auto rmEnd = std::remove_if(it->second.begin(), it->second.end(),
                              [&](auto& unique) { return unique.get() == ptr; });
  it->second.erase(rmEnd, it->second.end());
}
