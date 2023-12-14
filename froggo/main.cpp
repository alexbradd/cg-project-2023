#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <seng/application.hpp>
#include <seng/camera.hpp>
#include <seng/game_context.hpp>
#include <seng/input_enums.hpp>
#include <seng/input_manager.hpp>
#include <seng/log.hpp>
#include <seng/transform.hpp>

using namespace std;
namespace fs = std::filesystem;

int main(int, char* argv[])
{
  fs::path dir{fs::path{argv[0]}.parent_path()};

  seng::ApplicationConfig config{"Froggo", (dir / "shaders").string(),
                                 (dir / "assets").string()};
  seng::Application app(config);

  seng::log::info("Reading assets from {}", app.config().assetPath);
  seng::log::info("Reading shaders from {}", app.config().shaderPath);

  try {
    seng::log::info("Starting application");

    const float speed = 5.0f;
    app.run(800, 600, [&](const seng::GameContext* ctx) {
      /* auto& transform = ctx->currentCamera().transform(); */
      /**/
      /* if (ctx->inputManager()->keyHold(seng::KeyCode::eKeyA)) */
      /*   transform.translate(-transform.right() * speed * ctx->deltaTime().count()); */
      /* if (ctx->inputManager()->keyHold(seng::KeyCode::eKeyD)) */
      /*   transform.translate(transform.right() * speed * ctx->deltaTime().count()); */
      /* if (ctx->inputManager()->keyHold(seng::KeyCode::eKeyW)) */
      /*   transform.translate(-transform.forward() * speed * ctx->deltaTime().count()); */
      /* if (ctx->inputManager()->keyHold(seng::KeyCode::eKeyS)) */
      /*   transform.translate(transform.forward() * speed * ctx->deltaTime().count()); */
      /* if (ctx->inputManager()->keyHold(seng::KeyCode::eSpace)) { */
      /*   if (ctx->inputManager()->keyHold(seng::KeyCode::eModLeftShift)) */
      /*     transform.translate(-transform.up() * speed * ctx->deltaTime().count()); */
      /*   else */
      /*     transform.translate(transform.up() * speed * ctx->deltaTime().count()); */
      /* } */
      /**/
      /* if (ctx->inputManager()->keyHold(seng::KeyCode::eUp)) */
      /*   transform.rotate(glm::radians(1.0f), 0.0f, 0.0f); */
      /* if (ctx->inputManager()->keyHold(seng::KeyCode::eDown)) */
      /*   transform.rotate(glm::radians(-1.0f), 0.0f, 0.0f); */
      /* if (ctx->inputManager()->keyHold(seng::KeyCode::eLeft)) */
      /*   transform.rotate(0.0f, glm::radians(1.0f), 0.0f); */
      /* if (ctx->inputManager()->keyHold(seng::KeyCode::eRight)) */
      /*   transform.rotate(0.0f, glm::radians(-1.0f), 0.0f); */
    });
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return EXIT_SUCCESS;
}
