#include <seng/application.hpp>
#include <seng/log.hpp>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

int main(int, char* argv[])
{
  fs::path dir{fs::path{argv[0]}.parent_path()};

  seng::ApplicationConfig config{"Froggo", (dir / "shaders").string(),
                                 (dir / "assets").string(), (dir / "scenes").string()};
  seng::Application app(config);

  seng::log::info("Reading assets from {}", app.config().assetPath);
  seng::log::info("Reading shaders from {}", app.config().shaderPath);
  seng::log::info("Reading scenes from {}", app.config().scenePath);

  try {
    seng::log::info("Starting application");

    app.run(800, 600);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return EXIT_SUCCESS;
}
