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
  GameContext() {}
  GameContext(const GameContext&) = delete;
  GameContext(GameContext&&) = default;

  GameContext& operator=(const GameContext&) = delete;
  GameContext& operator=(GameContext&&) = default;

  const InputManager* inputManager() const { return _inputManager.get(); }
  std::chrono::duration<float> deltaTime() const { return _deltaTime; }

 private:
  std::unique_ptr<InputManager> _inputManager;
  std::chrono::duration<float> _deltaTime;
};

};  // namespace seng
