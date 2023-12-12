#pragma once

#include <chrono>
#include <memory>
#include <seng/application.hpp>

namespace seng {

class InputManager;
class Camera;

class GameContext {
  friend Application;  // To allow update of internal objects

 public:
  GameContext(Camera& c) : _currentCamera(c) {}
  GameContext(const GameContext&) = delete;
  GameContext(GameContext&&) = default;

  GameContext& operator=(const GameContext&) = delete;
  GameContext& operator=(GameContext&&) = default;

  std::shared_ptr<InputManager> inputManager() const { return _inputManager; }
  std::chrono::duration<float> deltaTime() const { return _deltaTime; }
  Camera& currentCamera() const { return _currentCamera; }

 private:
  std::shared_ptr<InputManager> _inputManager;
  std::chrono::duration<float> _deltaTime;
  std::reference_wrapper<Camera> _currentCamera;
};

};  // namespace seng
