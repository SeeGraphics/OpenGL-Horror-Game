#include <GLFW/glfw3.h>

#include <iostream>

#include "app.hpp"

// my external monitor is the one i want to view the game in, since its
// positioned above my laptop its Y value is higher, we pick the one with
// the highest Y
int main() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // for mac
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);

  // glfw window creation fullscreen on startup
  GLFWmonitor *monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode *mode = glfwGetVideoMode(monitor);
  GLFWwindow *window = glfwCreateWindow(mode->width, mode->height,
                                        "OpenGL Window", monitor, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(0); // no v-sync
  int windowWidth = 0;
  int windowHeight = 0;
  int framebufferWidth = 0;
  int framebufferHeight = 0;
  glfwGetWindowSize(window, &windowWidth, &windowHeight);
  glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
  std::cout << "Window size: " << windowWidth << "x" << windowHeight
            << " | Framebuffer size: " << framebufferWidth << "x"
            << framebufferHeight << std::endl;

  AppState app;
  if (!AppInit(app, window)) {
    glfwTerminate();
    return -1;
  }

  // render loop
  while (!glfwWindowShouldClose(window)) {
    AppFrame(app, window);
  }

  AppShutdown(app);

  // glfw: terminate, clearing all previously allocated GLFW resources.
  glfwTerminate();
  return 0;
}
