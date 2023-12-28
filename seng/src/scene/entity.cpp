#include <seng/log.hpp>
#include <seng/scene/entity.hpp>
#include <seng/transform.hpp>

#include <memory>

using namespace seng::scene;
using namespace seng::components;
using namespace std;

uint64_t Entity::INDEX_COUNTER = 0;

Entity::Entity(std::string n) :
    id(INDEX_COUNTER++), name(std::move(n)), transform(std::make_unique<Transform>())
{
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