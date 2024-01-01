#include "camera_orchestrator.hpp"
#include "test_controller.hpp"

#include <seng/application.hpp>
#include <seng/components/base_component.hpp>
#include <seng/components/camera.hpp>
#include <seng/components/script.hpp>
#include <seng/input_enums.hpp>
#include <seng/input_manager.hpp>
#include <seng/log.hpp>
#include <seng/scene/entity.hpp>

#include <yaml-cpp/yaml.h>

#include <memory>

using namespace seng;

CameraOrchestrator::CameraOrchestrator(Entity &entity) : ScriptComponent(entity) {}
std::unique_ptr<BaseComponent> CameraOrchestrator::createFromConfig(
    Entity &entity, [[maybe_unused]] const YAML::Node &node)
{
  return std::make_unique<CameraOrchestrator>(entity);
}

void CameraOrchestrator::scriptInitialize()
{
  auto entityCam1 = entity->scene().findByName("cam1");
  auto entityCam2 = entity->scene().findByName("cam2");

  m_cam1 = entity->scene().mainCamera();
  m_controller1 =
      entityCam1->componentsOfType<TestController>().front().sureGet<TestController>();

  m_cam2 = entityCam2->componentsOfType<Camera>().front().sureGet<Camera>();
  m_controller2 =
      entityCam2->componentsOfType<TestController>().front().sureGet<TestController>();

  m_input = entity->application().input().get();
}

void CameraOrchestrator::onUpdate([[maybe_unused]] float delta)
{
  if (m_input->keyDown(seng::KeyCode::eKeyC)) {
    m_controller1->toggle();
    m_controller2->toggle();

    if (m_controller1->enabled())
      entity->scene().mainCamera(m_cam1);
    else
      entity->scene().mainCamera(m_cam2);
  }
}
