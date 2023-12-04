#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <seng/application.hpp>
#include <seng/log.hpp>

using namespace std;
namespace fs = std::filesystem;

int main(int, char* argv[]) {
  fs::path dir{fs::path{argv[0]}.parent_path()};

  seng::ApplicationConfig config{"Froggo", (dir / "shaders").string(),
                                 (dir / "assets").string()};
  seng::Application app(config);

  seng::log::info("Reading assets from {}", app.config().assetPath);
  seng::log::info("Reading shaders from {}", app.config().shaderPath);

  try {
    seng::log::info("Starting application");
    app.run(800, 600, [](auto input) {
    });
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return EXIT_SUCCESS;
}
