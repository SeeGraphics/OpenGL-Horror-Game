#define GL_SILENCE_DEPRECATION
// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include "app.hpp"

#include "render/renderer.hpp"
#include "scene/world.hpp"
#include "ui/debugUi.hpp"
#include "ui/mapEditor.hpp"

static AppState *g_state = nullptr;

static void processInput(GLFWwindow *window);
static void mouse_callback(GLFWwindow *window, double xpos, double ypos);
static void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height);

bool AppInit(AppState &state, GLFWwindow *window) {
  g_state = &state;
  glfwSetCursorPosCallback(window, mouse_callback);

  if (!renderInit(state, window)) {
    return false;
  }

  initWorldModels(state);

  if (!initDebugUi(window)) {
    return false;
  }

  if (!initAudio(state.audio)) {
    return false;
  }
  // startLoopingSound(state.audio, SoundId::NightForestAmbient);

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  return true;
}

void AppFrame(AppState &state, GLFWwindow *window) {
  static bool wasGrounded = true;

  // calculate delta time
  float currentFrame = glfwGetTime();
  state.deltaTime = currentFrame - state.lastFrame;
  state.lastFrame = currentFrame;

  // input
  processInput(window);

  if (state.camera.flashlightToggled) {
    playSound(state.audio, SoundId::FlashlightToggle);
    state.camera.flashlightToggled = false;
  }

  bool hasUi = state.showDebugUi || state.editorEnabled;
  if (hasUi) {
    beginUiFrame();
    if (state.showDebugUi) {
      drawDebugUi(state);
    }
    if (state.editorEnabled) {
      drawMapEditorUi(state);
      handleEditorPicking(state, window);
      updateMapEditorGizmo(state, window);
    }
  }

  updateGroundCollision(state);
  updateFlashlightAttachment(state);

  if (!state.freeCam && !wasGrounded && state.camera.isGrounded) {
    playSound(state.audio, SoundId::LandingOnGrass);
  }

  if (!state.freeCam && state.camera.isGrounded && state.camera.isMoving) {
    if (state.camera.isSprinting) {
      startLoopingSound(state.audio, SoundId::RunningGrass);
      stopSound(state.audio, SoundId::StepOnGrass);
    } else {
      startLoopingSound(state.audio, SoundId::StepOnGrass);
      stopSound(state.audio, SoundId::RunningGrass);
    }
  } else {
    stopSound(state.audio, SoundId::RunningGrass);
    stopSound(state.audio, SoundId::StepOnGrass);
  }

  wasGrounded = state.camera.isGrounded;

  renderFrame(state, window);

  // render imgui
  if (hasUi) {
    endDebugUiFrame();
  }

  // glfw: swap buffers and poll IO events
  glfwSwapBuffers(window);
  glfwPollEvents();
}

void AppShutdown(AppState &state) {
  renderShutdown(state);
  shutdownDebugUi();
  shutdownAudio(state.audio);
  g_state = nullptr;
}

static void processInput(GLFWwindow *window) {
  if (!g_state)
    return;
  AppState &state = *g_state;

  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  // toggle editor
  static bool eWasDown = false;
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
    if (!eWasDown) {
      state.editorEnabled = !state.editorEnabled;
      state.freeCam = !state.freeCam;
      if (state.editorEnabled) {
        state.editorSavedRenderDistance = state.renderDistance;
        state.editorSavedAmbientStrength = state.ambientStrength;
        state.editorSavedSkyboxIntensity = state.skyboxIntensity;
        state.editorSavedFogDensity = state.fogDensity;
        state.editorHasSavedValues = true;

        state.renderDistance = 2000.0f; // make sure we see whole map
        state.ambientStrength = 2.5f;
        state.skyboxIntensity = 1.0f;
        state.fogDensity = 0.0f;
      } else if (state.editorHasSavedValues) {
        state.renderDistance = state.editorSavedRenderDistance;
        state.ambientStrength = state.editorSavedAmbientStrength;
        state.skyboxIntensity = state.editorSavedSkyboxIntensity;
        state.fogDensity = state.editorSavedFogDensity;
      }
    }
    eWasDown = true;
  } else {
    eWasDown = false;
  }

  // toggle freeCam
  static bool vWasDown = false;
  if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
    if (!vWasDown) {
      state.freeCam = !state.freeCam; // Toggle only once per press
    }
    vWasDown = true;
  } else {
    vWasDown = false;
  }

  if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
    if (!state.fullscreen) {
      // Switch to fullscreen
      GLFWmonitor *monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode *mode = glfwGetVideoMode(monitor);
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
  state.camera.ProcessKeyboard(state, window, state.deltaTime, state.freeCam);
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

  // toggle debug UI
  static bool gWasDown = false;
  if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
    if (!gWasDown) {
      state.showDebugUi = !state.showDebugUi;
    }
    gWasDown = true;
  } else {
    gWasDown = false;
  }
}

static void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  (void)window;
  if (!g_state)
    return;
  if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
    g_state->camera.ProcessMouse(xpos, ypos);
  }
}

static void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height) {
  (void)window;
  glViewport(0, 0, width, height);
}
