#define GL_SILENCE_DEPRECATION
// clang-format off
#include <GLFW/glfw3.h>
// clang-format on
#include "app.hpp"
#include "scene/world.hpp"
#include "render/renderer.hpp"
#include "ui/debugUi.hpp"

static AppState* g_state = nullptr;

static void processInput(GLFWwindow* window);
static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
static void framebuffer_size_callback(GLFWwindow* window, int width,
                                       int height);

bool AppInit(AppState& state, GLFWwindow* window) {
  g_state = &state;
  glfwSetCursorPosCallback(window, mouse_callback);

  if (!renderInit(state, window)) {
    return false;
  }

  if (!initDebugUi(window)) {
    return false;
  }

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  return true;
}

void AppFrame(AppState& state, GLFWwindow* window) {
  // calculate delta time
  float currentFrame = glfwGetTime();
  state.deltaTime = currentFrame - state.lastFrame;
  state.lastFrame = currentFrame;

  // input
  processInput(window);

  beginDebugUiFrame();
  drawDebugUi(state);

  updateGroundCollision(state);

  renderFrame(state, window);

  // render imgui
  endDebugUiFrame();

  // glfw: swap buffers and poll IO events
  glfwSwapBuffers(window);
  glfwPollEvents();
}

void AppShutdown(AppState& state) {
  renderShutdown(state);

  shutdownDebugUi();

  g_state = nullptr;
}

static void processInput(GLFWwindow* window) {
  if (!g_state) return;
  AppState& state = *g_state;

  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  // toggle freeCam
  static bool vWasDown = false;
  if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
    if (!vWasDown) {
      state.freeCam = !state.freeCam;  // Toggle only once per press
    }
    vWasDown = true;
  } else {
    vWasDown = false;
  }

  if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
    if (!state.fullscreen) {
      // Switch to fullscreen
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height,
                           mode->refreshRate);
      state.fullscreen = true;
    } else {
      // Switch back to windowed
      glfwSetWindowMonitor(window, NULL, 100, 100, 800, 600, 0);
      state.fullscreen = false;
    }
  }

  // player/ camera controls from camera.cpp
  state.camera.ProcessKeyboard(window, state.deltaTime, state.freeCam);
  static bool pWasDown = false;

  // toggle Wireframe mode
  if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
    if (!pWasDown) {
      state.wireframe = !state.wireframe;
    }
    pWasDown = true;
  } else {
    pWasDown = false;
  }
}

static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
  (void)window;
  if (!g_state) return;
  if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
    g_state->camera.ProcessMouse(xpos, ypos);
  }
}

static void framebuffer_size_callback(GLFWwindow* window, int width,
                                       int height) {
  (void)window;
  glViewport(0, 0, width, height);
}
