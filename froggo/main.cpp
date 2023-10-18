#include <cstdlib>
#include <exception>
#include <iostream>

#include <seng/application.hpp>
#include <seng/log.hpp>

int main (int argc, char *argv[]) {
  seng::Application app {"Froggo", 800, 600};

  try {
    seng::log::info("Starting application");
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return EXIT_SUCCESS;
}
