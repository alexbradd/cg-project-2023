#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <seng/application.hpp>
#include <seng/log.hpp>

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
  fs::path dir{fs::path{argv[0]}.parent_path()};

  seng::Application app{"Froggo", 800, 600};

  app.setModelPath((dir / "models").string());
  app.setShaderPath((dir / "shaders").string());
  seng::log::info("Reading models from {}", app.getModelPath());
  seng::log::info("Reading shaders from {}", app.getShaderPath());

  try {
    seng::log::info("Starting application");
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return EXIT_SUCCESS;
}
