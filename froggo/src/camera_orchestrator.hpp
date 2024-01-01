#pragma once

#include "test_controller.hpp"

#include <seng/components/definitions.hpp>
#include <seng/components/scene_config_component_factory.hpp>
#include <seng/components/script.hpp>

class CameraOrchestrator : public seng::ScriptComponent,
                           public seng::ConfigParsableComponent<CameraOrchestrator> {
 public:
  CameraOrchestrator(seng::Entity &entity);
  CameraOrchestrator(const CameraOrchestrator &) = delete;
  CameraOrchestrator(CameraOrchestrator &&) = delete;

  CameraOrchestrator &operator=(const CameraOrchestrator &) = delete;
  CameraOrchestrator &operator=(CameraOrchestrator &&) = delete;

  DECLARE_COMPONENT_ID("CameraOrchestrator");
  static std::unique_ptr<seng::BaseComponent> createFromConfig(seng::Entity &entity,
                                                               const YAML::Node &node);

  void onUpdate(float delta) override;

 private:
  seng::Camera *m_cam1;
  seng::Camera *m_cam2;

  TestController *m_controller1;
  TestController *m_controller2;

  seng::InputManager *m_input;
};
