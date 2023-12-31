#pragma once

#include "test_controller.hpp"

#include <seng/components/definitions.hpp>
#include <seng/components/scene_config_component_factory.hpp>
#include <seng/components/script.hpp>

class CameraOrchestrator
    : public seng::components::ScriptComponent,
      public seng::components::ConfigParsableComponent<CameraOrchestrator> {
 public:
  CameraOrchestrator(seng::scene::Entity &entity);
  CameraOrchestrator(const CameraOrchestrator &) = delete;
  CameraOrchestrator(CameraOrchestrator &&) = delete;

  CameraOrchestrator &operator=(const CameraOrchestrator &) = delete;
  CameraOrchestrator &operator=(CameraOrchestrator &&) = delete;

  DECLARE_COMPONENT_ID("CameraOrchestrator");
  static std::unique_ptr<seng::components::BaseComponent> createFromConfig(
      seng::scene::Entity &entity, const YAML::Node &node);

  void scriptInitialize() override;
  void onUpdate(float delta) override;

 private:
  seng::components::Camera *m_cam1;
  seng::components::Camera *m_cam2;

  TestController *m_controller1;
  TestController *m_controller2;

  seng::InputManager *m_input;
};
