#pragma once

#include "./car_controller.hpp"

#include <seng/components/definitions.hpp>
#include <seng/components/scene_config_component_factory.hpp>
#include <seng/components/script.hpp>
#include <seng/components/transform.hpp>

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
