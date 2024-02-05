#include <seng/components/definitions.hpp>
#include <seng/components/scene_config_component_factory.hpp>
#include <seng/components/script.hpp>
#include <seng/components/transform.hpp>

#include <yaml-cpp/yaml.h>

class CarCamera : public seng::ScriptComponent,
                  public seng::ConfigParsableComponent<CarCamera> {
 public:
  CarCamera(seng::Entity &entity, float hDistance, float yDistance, bool enabled = true);
  CarCamera(const CarCamera &) = delete;
  CarCamera(CarCamera &&) = delete;

  CarCamera &operator=(const CarCamera &) = delete;
  CarCamera &operator=(CarCamera &&) = delete;

  DECLARE_COMPONENT_ID("CarCamera");
  DECLARE_CREATE_FROM_CONFIG();

  void onUpdate(float deltaTime) override;

 private:
  float m_z, m_y;

  seng::Transform *m_car;
};

REGISTER_TO_CONFIG_FACTORY(CarCamera);

DEFINE_CREATE_FROM_CONFIG(CarCamera, entity, node)
{
  bool enabled = true;
  float xDistance = 10.0f;
  float yDistance = 10.0f;

  if (node["enabled"]) enabled = node["enabled"].as<bool>();
  if (node["h_distance"]) xDistance = node["h_distance"].as<float>();
  if (node["v_distance"]) yDistance = node["v_distance"].as<float>();
  return std::make_unique<CarCamera>(entity, xDistance, yDistance, enabled);
}

CarCamera::CarCamera(seng::Entity &entity,
                     float hDistance,
                     float vDistance,
                     bool enabled) :
    seng::ScriptComponent(entity, enabled)
{
  m_z = hDistance;
  m_y = vDistance;

  m_car = entity.scene().findByName("car")->transform();
}

void CarCamera::onUpdate([[maybe_unused]] float delta)
{
  glm::vec3 carPos = m_car->position();
  glm::vec3 newPos = {carPos.x, carPos.y + m_y, carPos.z - m_z};

  entity->transform()->position(newPos);
  entity->transform()->lookAt(*m_car);
}
