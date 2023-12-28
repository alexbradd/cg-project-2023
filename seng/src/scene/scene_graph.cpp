#include <seng/log.hpp>
#include <seng/scene/scene_graph.hpp>

/* using namespace seng; */
using namespace seng::scene;
using namespace std;

SceneGraph::EntityList::const_iterator SceneGraph::findByName(
    const std::string &name) const
{
  return std::find_if(entities.begin(), entities.end(),
                      [&](const auto &elem) { return elem.getName() == name; });
}

SceneGraph::EntityList::iterator SceneGraph::findByName(const std::string &name)
{
  return std::find_if(entities.begin(), entities.end(),
                      [&](const auto &elem) { return elem.getName() == name; });
}

Entity *SceneGraph::newEntity(std::string name)
{
  entities.push_back(Entity(name));
  return &entities.back();
}

void SceneGraph::remove(EntityList::const_iterator i)
{
  entities.erase(i);
}

void SceneGraph::remove(const Entity *e)
{
  if (e == nullptr) {
    seng::log::warning("Tried to remove a null entity... Something is wrong");
    return;
  }
  auto it = std::find(entities.begin(), entities.end(), *e);
  if (it == entities.end()) {
    seng::log::warning(
        "Tried to remove an entity not registered in the scene graph... Something is "
        "wrong");
    return;
  }
  this->remove(it);
}

void SceneGraph::remove(const string &name)
{
  auto it = findByName(name);
  if (it == entities.end()) {
    seng::log::warning("Could not find an entity with the given name to remove");
    return;
  }
  this->remove(it);
}

void SceneGraph::clear()
{
  entities.clear();
}
