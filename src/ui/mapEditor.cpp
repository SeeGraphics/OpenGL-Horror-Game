#include "ui/mapEditor.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ImGuizmo.h"

#include "app.hpp"

static bool getMouseRay(const AppState &state, GLFWwindow *window,
                        glm::vec3 &origin, glm::vec3 &direction);
static int pickModelInstance(const AppState &state, const glm::vec3 &origin,
                             const glm::vec3 &direction);
static void buildInstanceMatrix(const ModelInstance &instance,
                                glm::mat4 &modelMatrix);
static ImGuizmo::OPERATION getGizmoOperation(AppState &state);
static void updateGizmoHotkeys(AppState &state, GLFWwindow *window);

void handleEditorPicking(AppState &state, GLFWwindow *window) {
  if (!state.editorEnabled) {
    return;
  }
  if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_NORMAL) {
    return;
  }
  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureMouse) {
    return;
  }
  if (ImGuizmo::IsOver() || ImGuizmo::IsUsing()) {
    return;
  }

  static bool leftWasDown = false;
  int leftState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
  if (leftState == GLFW_PRESS && !leftWasDown) {
    glm::vec3 rayOrigin(0.0f);
    glm::vec3 rayDir(0.0f);
    if (getMouseRay(state, window, rayOrigin, rayDir)) {
      int hitIndex = pickModelInstance(state, rayOrigin, rayDir);
      if (hitIndex >= 0) {
        state.selectedInstance = hitIndex;
        const ModelInstance &instance = state.modelInstances[hitIndex];
        const ModelAsset &asset = state.modelAssets[instance.assetIndex];
        const std::string &name = asset.id.empty() ? asset.path : asset.id;
        std::cout << "Selected model: " << name << std::endl;
      }
    }
  }
  leftWasDown = (leftState == GLFW_PRESS);
}

void updateMapEditorGizmo(AppState &state, GLFWwindow *window) {
  if (!state.editorEnabled) {
    return;
  }
  if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_NORMAL) {
    return;
  }
  if (state.selectedInstance < 0 ||
      state.selectedInstance >= static_cast<int>(state.modelInstances.size())) {
    return;
  }

  ModelInstance &instance = state.modelInstances[state.selectedInstance];
  if (instance.assetIndex < 0 ||
      instance.assetIndex >= static_cast<int>(state.modelAssets.size())) {
    return;
  }

  const ModelAsset &asset = state.modelAssets[instance.assetIndex];
  if (!asset.model.IsLoaded()) {
    return;
  }

  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureKeyboard) {
    return;
  }
  updateGizmoHotkeys(state, window);
  ImGuizmo::BeginFrame();
  ImGuizmo::SetOrthographic(false);
  ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
  ImGuizmo::SetRect(0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y);

  glm::mat4 view = state.camera.GetViewMatrix();
  float aspect =
      (io.DisplaySize.y > 0.0f) ? (io.DisplaySize.x / io.DisplaySize.y) : 1.0f;
  glm::mat4 projection =
      glm::perspective(glm::radians(60.0f), aspect, 0.1f, state.renderDistance);

  glm::mat4 modelMatrix = glm::mat4(1.0f);
  buildInstanceMatrix(instance, modelMatrix);

  ImGuizmo::OPERATION operation = getGizmoOperation(state);
  bool changed = ImGuizmo::Manipulate(
      glm::value_ptr(view), glm::value_ptr(projection), operation,
      ImGuizmo::WORLD, glm::value_ptr(modelMatrix));

  bool usingGizmo = ImGuizmo::IsUsing();
  if (changed || usingGizmo) {
    float translation[3] = {};
    float rotation[3] = {};
    float scale[3] = {};
    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(modelMatrix),
                                          translation, rotation, scale);
    instance.position =
        glm::vec3(translation[0], translation[1], translation[2]);
    instance.rotation = glm::vec3(rotation[0], rotation[1], rotation[2]);
    instance.scale = glm::vec3(scale[0], scale[1], scale[2]);
    if (instance.assetIndex == state.treeAssetIndex) {
      state.treeInstanceDirty = true;
    }
  }

  static bool wasUsing = false;
  if (wasUsing && !usingGizmo && instance.freeArea) {
    state.treeDirty = true;
  }
  wasUsing = usingGizmo;
}

static bool getMouseRay(const AppState &state, GLFWwindow *window,
                        glm::vec3 &origin, glm::vec3 &direction) {
  double mouseX = 0.0;
  double mouseY = 0.0;
  glfwGetCursorPos(window, &mouseX, &mouseY);
  int width = 0;
  int height = 0;
  glfwGetWindowSize(window, &width, &height);
  if (width <= 0 || height <= 0) {
    return false;
  }

  float xNdc = (2.0f * static_cast<float>(mouseX) / width) - 1.0f;
  float yNdc = 1.0f - (2.0f * static_cast<float>(mouseY) / height);

  glm::mat4 view = state.camera.GetViewMatrix();
  float aspect = static_cast<float>(width) / static_cast<float>(height);
  glm::mat4 projection =
      glm::perspective(glm::radians(60.0f), aspect, 0.1f, state.renderDistance);
  glm::mat4 invViewProj = glm::inverse(projection * view);

  glm::vec4 nearPoint = invViewProj * glm::vec4(xNdc, yNdc, -1.0f, 1.0f);
  glm::vec4 farPoint = invViewProj * glm::vec4(xNdc, yNdc, 1.0f, 1.0f);
  if (nearPoint.w == 0.0f || farPoint.w == 0.0f) {
    return false;
  }
  nearPoint /= nearPoint.w;
  farPoint /= farPoint.w;

  origin = glm::vec3(nearPoint);
  direction = glm::normalize(glm::vec3(farPoint - nearPoint));
  return true;
}

static int pickModelInstance(const AppState &state, const glm::vec3 &origin,
                             const glm::vec3 &direction) {
  float bestT = std::numeric_limits<float>::max();
  int bestIndex = -1;

  for (int i = 0; i < static_cast<int>(state.modelInstances.size()); ++i) {
    const ModelInstance &instance = state.modelInstances[i];
    if (instance.assetIndex < 0 ||
        instance.assetIndex >= static_cast<int>(state.modelAssets.size())) {
      continue;
    }
    if ((!state.flashlightShown || state.editorEnabled) &&
        instance.assetIndex == state.flashlightAssetIndex &&
        !instance.isEditorPlaced) {
      continue;
    }

    const ModelAsset &asset = state.modelAssets[instance.assetIndex];
    if (!asset.model.IsLoaded()) {
      continue;
    }

    float radius = asset.model.GetBoundingRadius();
    if (radius <= 0.0f) {
      continue;
    }
    float scale =
        std::max({instance.scale.x, instance.scale.y, instance.scale.z});
    radius *= scale;
    if (radius <= 0.0f) {
      continue;
    }

    glm::vec3 oc = origin - instance.position;
    float b = glm::dot(oc, direction);
    float c = glm::dot(oc, oc) - (radius * radius);
    float h = (b * b) - c;
    if (h < 0.0f) {
      continue;
    }

    float sqrtH = std::sqrt(h);
    float t = -b - sqrtH;
    if (t < 0.0f) {
      t = -b + sqrtH;
    }
    if (t < 0.0f) {
      continue;
    }
    if (t < bestT) {
      bestT = t;
      bestIndex = i;
    }
  }

  return bestIndex;
}

static void buildInstanceMatrix(const ModelInstance &instance,
                                glm::mat4 &modelMatrix) {
  modelMatrix = glm::mat4(1.0f);
  modelMatrix = glm::translate(modelMatrix, instance.position);
  modelMatrix = glm::rotate(modelMatrix, glm::radians(instance.rotation.y),
                            glm::vec3(0.0f, 1.0f, 0.0f));
  modelMatrix = glm::rotate(modelMatrix, glm::radians(instance.rotation.x),
                            glm::vec3(1.0f, 0.0f, 0.0f));
  modelMatrix = glm::rotate(modelMatrix, glm::radians(instance.rotation.z),
                            glm::vec3(0.0f, 0.0f, 1.0f));
  modelMatrix = glm::scale(modelMatrix, instance.scale);
}

static ImGuizmo::OPERATION getGizmoOperation(AppState &state) {
  if (state.gizmoOperation < 0) {
    state.gizmoOperation = GizmoTranslate;
  }

  switch (state.gizmoOperation) {
  case GizmoRotate:
    return ImGuizmo::ROTATE;
  case GizmoScale:
    return ImGuizmo::SCALE;
  case GizmoTranslate:
  default:
    return ImGuizmo::TRANSLATE;
  }
}

static void updateGizmoHotkeys(AppState &state, GLFWwindow *window) {
  static bool oneWasDown = false;
  static bool twoWasDown = false;
  static bool threeWasDown = false;

  if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
    if (!oneWasDown) {
      state.gizmoOperation = GizmoTranslate;
    }
    oneWasDown = true;
  } else {
    oneWasDown = false;
  }

  if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
    if (!twoWasDown) {
      state.gizmoOperation = GizmoRotate;
    }
    twoWasDown = true;
  } else {
    twoWasDown = false;
  }

  if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
    if (!threeWasDown) {
      state.gizmoOperation = GizmoScale;
    }
    threeWasDown = true;
  } else {
    threeWasDown = false;
  }
}
