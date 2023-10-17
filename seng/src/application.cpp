#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <seng/application.hpp>

using namespace std;

void seng::Application::run() {
  initWindow();
  initVulkan();
  mainLoop();
  cleanup();
}

void seng::Application::initWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE,
                 GLFW_FALSE); // FIXME: make it resizeable when ready
  this->w = glfwCreateWindow(this->width,
                             this->height,
                             this->w_name.c_str(),
                             nullptr, nullptr);
}

void seng::Application::initVulkan() {}

void seng::Application::mainLoop() {
  while (!glfwWindowShouldClose(this->w)) {
    glfwPollEvents();
  }
}

void seng::Application::cleanup() {
  glfwDestroyWindow(this->w);
  glfwTerminate();
}
