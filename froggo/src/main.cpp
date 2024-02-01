#include <seng/application.hpp>
#include <seng/log.hpp>

#include <cstdlib>
#include <exception>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

int main(int, char* argv[])
{
  fs::path dir{fs::path{argv[0]}.parent_path()};

  seng::ApplicationConfig config;
  config.appName = "Froggo";
  config.shaderDefinitions = (dir / "shaders" / "shaders.yml").string();
  config.shaderPath = (dir / "shaders").string();
  config.assetPath = (dir / "assets").string();
  config.scenePath = (dir / "scenes").string();

  // Color: #abf6fc
  config.clearColorRed = 0.617;
  config.clearColorGreen = 0.965;
  config.clearColorBlue = 0.988;

  seng::Application app(config);

  seng::log::info("Reading assets from {}", app.config().assetPath);
  seng::log::info("Reading shaders from {}", app.config().shaderPath);
  seng::log::info("Reading scenes from {}", app.config().scenePath);

  try {
    seng::log::info("Starting application");
    app.run(800, 600);
    return EXIT_SUCCESS;
  } catch (const std::exception& e) {
    seng::log::error("Fatal error encountered: {}", e.what());
    return EXIT_FAILURE;
  }
}
