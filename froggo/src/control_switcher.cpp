#include "./car_camera.hpp"
#include "./car_controller.hpp"
#include "seng/input_enums.hpp"

#include <seng/application.hpp>
#include <seng/components/definitions.hpp>
#include <seng/components/free_controller.hpp>
#include <seng/components/scene_config_component_factory.hpp>
#include <seng/components/script.hpp>
#include <seng/components/transform.hpp>
#include <seng/input_manager.hpp>

#include <yaml-cpp/yaml.h>

#include <memory>

class ControlSwitcher : public seng::ScriptComponent,
                        public seng::ConfigParsableComponent<ControlSwitcher> {
 public:
  ControlSwitcher(seng::Entity &entity,
                  const std::string &controllerEntity,
                  const std::string &cameraEntity,
                  bool enabled = true);
  ControlSwitcher(const ControlSwitcher &) = delete;
  ControlSwitcher(ControlSwitcher &&) = delete;

  ControlSwitcher &operator=(const ControlSwitcher &) = delete;
  ControlSwitcher &operator=(ControlSwitcher &&) = delete;

  DECLARE_COMPONENT_ID("ControlSwitcher");
  DECLARE_CREATE_FROM_CONFIG();

  void onUpdate(float deltaTime) override;

 private:
  CarController *m_carController;
  CarCamera *m_carCamera;
  seng::FreeController *m_freeController;

  seng::Transform *m_camera;

  bool m_free = false;
  glm::vec3 m_localPosition;
};

REGISTER_TO_CONFIG_FACTORY(ControlSwitcher);

DEFINE_CREATE_FROM_CONFIG(ControlSwitcher, entity, node)
{
  bool enabled = true;
  std::string controller, camera;

  camera = node["camera_entity"].as<std::string>();
  controller = node["controller_entity"].as<std::string>();
  if (node["enabled"]) enabled = node["enabled"].as<bool>();

  return std::make_unique<ControlSwitcher>(entity, controller, camera, enabled);
}

ControlSwitcher::ControlSwitcher(seng::Entity &entity,
                                 const std::string &controllerEntity,
                                 const std::string &cameraEntity,
                                 bool enabled) :
    seng::ScriptComponent(entity, enabled)
{
  auto it = entity.scene().findByName(controllerEntity);
  if (it == entity.scene().entities().end())
    throw std::runtime_error("No entity named " + controllerEntity + " can be found");

  auto &carControllers = it->componentsOfType<CarController>();
  if (carControllers.empty())
    throw std::runtime_error("No CarController on entity named " + controllerEntity);
  m_carController = carControllers[0].sureGet<CarController>();

  it = entity.scene().findByName(cameraEntity);
  if (it == entity.scene().entities().end())
    throw std::runtime_error("No entity named " + cameraEntity + " can be found");
  m_camera = it->transform();

  auto &carCameras = it->componentsOfType<CarCamera>();
  if (carCameras.empty())
    throw std::runtime_error("No CarCamera on entity named " + cameraEntity);
  m_carCamera = carCameras[0].sureGet<CarCamera>();

  auto &freeCameras = it->componentsOfType<seng::FreeController>();
  if (freeCameras.empty())
    throw std::runtime_error("No FreeController on entity named " + cameraEntity);
  m_freeController = freeCameras[0].sureGet<seng::FreeController>();

  m_localPosition = entity.transform()->position();
}

void ControlSwitcher::onUpdate([[maybe_unused]] float delta)
{
  // If tab has been pressed, toggle behaviour
  if (!entity->application().input()->keyDown(seng::KeyCode::eTab)) return;

  if (m_free) {
    if (!m_carCamera->enabled() || !m_carController->enabled()) {
      // Restore the local posistion
      m_camera->position(m_localPosition);

      // Enable car controls
      m_carController->enable();
      m_carCamera->enable();

      // Disable free controls
      m_freeController->disable();

      m_free = false;
    }
  } else {
    if (!m_freeController->enabled()) {
      // Save the local posistion
      m_localPosition = m_camera->position();

      // Disable car controls
      m_carController->disable();
      m_carCamera->disable();

      // Enable free controls
      m_freeController->enable();

      m_free = true;
    }
  }
}
