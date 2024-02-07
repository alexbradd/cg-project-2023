#include "./car_controller.hpp"

#include <seng/components/camera.hpp>
#include <seng/components/definitions.hpp>
#include <seng/components/scene_config_component_factory.hpp>
#include <seng/components/script.hpp>
#include <seng/components/transform.hpp>
#include <seng/math.hpp>
#include <seng/scene/entity.hpp>

#include <yaml-cpp/yaml.h>
#include <glm/trigonometric.hpp>

#include <stdexcept>
#include <string>

using seng::smoothDamp;

class CarCamera : public seng::ScriptComponent,
                  public seng::ConfigParsableComponent<CarCamera> {
 public:
  CarCamera(seng::Entity &entity,
            const std::string &lookat,
            const std::string &controller,
            bool enabled = true);
  CarCamera(const CarCamera &) = delete;
  CarCamera(CarCamera &&) = delete;

  CarCamera &operator=(const CarCamera &) = delete;
  CarCamera &operator=(CarCamera &&) = delete;

  DECLARE_COMPONENT_ID("CarCamera");
  DECLARE_CREATE_FROM_CONFIG();

  void onUpdate(float deltaTime) override;

 private:
  seng::Transform *m_lookat;
  CarController *m_controller;
  seng::Camera *m_cam;

  float m_speedThresh;

  float m_cacheFov;
  float m_slowFov = glm::radians(45.0f);
  float m_fastFov = glm::radians(50.0f);
  float m_fovVelocity = 0.0f;
};

REGISTER_TO_CONFIG_FACTORY(CarCamera);

DEFINE_CREATE_FROM_CONFIG(CarCamera, entity, node)
{
  bool enabled = true;
  std::string lookat;
  std::string controller;

  if (node["enabled"]) enabled = node["enabled"].as<bool>();
  lookat = node["lookat_entity"].as<std::string>();
  controller = node["controller_entity"].as<std::string>();
  return std::make_unique<CarCamera>(entity, lookat, controller, enabled);
}

CarCamera::CarCamera(seng::Entity &entity,
                     const std::string &lookat,
                     const std::string &controller,
                     bool enabled) :
    seng::ScriptComponent(entity, enabled)
{
  auto it = entity.scene().findByName(lookat);
  if (it == entity.scene().entities().end())
    throw std::runtime_error("No entity named " + lookat + " can be found");
  m_lookat = it->transform();

  it = entity.scene().findByName(controller);
  if (it == entity.scene().entities().end())
    throw std::runtime_error("No entity named " + controller + " can be found");

  auto &controllers = it->componentsOfType<CarController>();
  if (controllers.empty())
    throw std::runtime_error("No CarController can be found on " + controller);
  m_controller = controllers[0].sureGet<CarController>();

  m_speedThresh = m_controller->maxSpeed() - m_controller->maxSpeed() / 2;

  auto &cams = entity.componentsOfType<seng::Camera>();
  if (cams.empty()) throw std::runtime_error("No Camera can be found on this entity");
  m_cam = cams[0].sureGet<seng::Camera>();
  m_cacheFov = m_cam->fov();
}

void CarCamera::onUpdate(float delta)
{
  entity->transform()->lookAt(*m_lookat);
  if (m_controller->speed() >= m_speedThresh)
    m_cacheFov = smoothDamp(m_cacheFov, m_fastFov, m_fovVelocity, 1.0f, delta);
  else
    m_cacheFov = smoothDamp(m_cacheFov, m_slowFov, m_fovVelocity, 1.0f, delta);
  if (m_cacheFov != m_cam->fov()) m_cam->fov(m_cacheFov);
}
