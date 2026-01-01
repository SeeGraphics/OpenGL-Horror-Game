#include "ui/debugUi.hpp"

#include <GLFW/glfw3.h>

#include "app.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "scene/world.hpp"

bool initDebugUi(GLFWwindow* window) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
  return true;
}

void beginUiFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void drawDebugUi(AppState& state) {
  ImGuiIO& io = ImGui::GetIO();

  ImGui::Begin("Info");
  ImGui::Text("FPS: %.f", io.Framerate);
  ImGui::Text("Player X: %.2f", state.camera.cameraPos.x);
  ImGui::Text("Player Y: %.2f", state.camera.cameraPos.y);
  ImGui::Text("Player Z: %.2f", state.camera.cameraPos.z);
  ImGui::End();

  ImGui::Begin("Settings");
  ImGui::Checkbox("Free Cam", &state.freeCam);
  ImGui::Checkbox("Wireframe", &state.wireframe);
  ImGui::Checkbox("Flashlight", &state.camera.flashlightEnabled);
  ImGui::PushItemWidth(50);
  ImGui::SliderFloat("Render Distance", &state.renderDistance, 5.0f, 1000.0f);
  ImGui::PopItemWidth();
  ImGui::End();

  ImGui::Begin("Environment");
  ImGui::SliderFloat("Ambient", &state.ambientStrength, 0.01f, 10.0f);
  ImGui::SliderFloat("Diffuse", &state.diffuseStrength, 0.01f, 10.0f);
  ImGui::SliderFloat("Specular", &state.specularStrength, 0.01f, 10.0f);
  ImGui::SliderFloat("Shininess", &state.shininess, 1.0f, 100.0f);
  ImGui::SliderFloat("Flashlight Brightness", &state.flashlightBrightness, 0.0f,
                     10.0f);
  ImGui::SliderFloat("Flashlight Radius (deg)", &state.flashlightRadius, 1.0f,
                     60.0f);
  ImGui::SliderFloat("Flashlight Offset Forward",
                     &state.flashlightOffsetForward, -2.0f, 2.0f);
  ImGui::SliderFloat("Flashlight Offset Right", &state.flashlightOffsetRight,
                     -2.0f, 2.0f);
  ImGui::SliderFloat("Flashlight Offset Down", &state.flashlightOffsetDown,
                     -2.0f, 2.0f);
  ImGui::SliderFloat3("Flashlight Beam Offset", &state.flashlightBeamOffset.x,
                      -2.0f, 2.0f);
  ImGui::SliderFloat3("Flashlight Beam Forward", &state.flashlightBeamForward.x,
                      -1.0f, 1.0f);
  ImGui::SliderFloat("Fog", &state.fogDensity, 0.0f, 0.50f);
  if (ImGui::SliderFloat("Heightmap Scale", &state.heightmapScale, 0.0f,
                         50.0f)) {
    state.terrainDirty = true;
    state.grassDirty = true;
  }
  if (ImGui::SliderFloat("Grass Density", &state.grassDensity, 0.0f, 5.0f)) {
    state.grassDirty = true;
  }
  if (ImGui::SliderFloat("Grass Radius", &state.grassRenderRadius, 0.0f,
                         state.renderDistance)) {
    state.grassDirty = true;
  }
  if (ImGui::SliderFloat("Tree Density", &state.treeDensity, 0.0f, 8.0f)) {
    state.treeDirty = true;
  }
  if (ImGui::SliderFloat("Tree Render Radius", &state.treeRenderRadius, 5.0f,
                         state.renderDistance)) {
    state.treeInstanceDirty = true;
  }
  if (ImGui::SliderFloat("Tree Update Distance", &state.treeUpdateDistance,
                         1.0f, 50.0f)) {
    state.treeInstanceDirty = true;
  }
  if (state.treeAssetIndex >= 0) {
    float treeScale = 0.0f;
    bool foundTree = false;
    for (const ModelInstance& instance : state.modelInstances) {
      if (instance.assetIndex == state.treeAssetIndex) {
        treeScale = instance.scale.x;
        foundTree = true;
        break;
      }
    }
    if (foundTree) {
      if (ImGui::SliderFloat("Tree Scale", &treeScale, 0.001f, 0.010f)) {
        for (ModelInstance& instance : state.modelInstances) {
          if (instance.assetIndex == state.treeAssetIndex) {
            instance.scale = glm::vec3(treeScale);
          }
        }
        state.treeInstanceDirty = true;
      }
    }
  }
  if (state.walterAssetIndex >= 0) {
    float walterScale = 0.0f;
    bool foundWalter = false;
    for (const ModelInstance& instance : state.modelInstances) {
      if (instance.assetIndex == state.walterAssetIndex) {
        walterScale = instance.scale.x;
        foundWalter = true;
        break;
      }
    }
    if (foundWalter) {
      if (ImGui::SliderFloat("Walter Scale", &walterScale, 0.0001f, 0.1f)) {
        for (ModelInstance& instance : state.modelInstances) {
          if (instance.assetIndex == state.walterAssetIndex) {
            instance.scale = glm::vec3(walterScale);
          }
        }
      }
    }
  }
  if (state.treeAssetIndex >= 0 &&
      state.treeAssetIndex < static_cast<int>(state.modelAssets.size())) {
    ModelRenderSettings& treeSettings =
        state.modelAssets[state.treeAssetIndex].renderSettings;
    ImGui::SliderFloat("Tree Intensity", &treeSettings.albedoIntensity, 0.0f,
                       2.0f);
    ImGui::SliderFloat("Tree Normal Strength", &treeSettings.normalStrength,
                       0.0f, 4.0f);
    ImGui::Checkbox("Tree Normal Debug", &treeSettings.normalDebug);
  }
  ImGui::SliderFloat("Grass Intensity", &state.grassIntensity, 0.0f, 1.0f);
  ImGui::SliderFloat("Skybox Intensity", &state.skyboxIntensity, 0.0f, 1.0f);
  ImGui::End();

  ImGui::Begin("Audio");
  if (ImGui::SliderFloat("Master Volume", &state.audio.masterVolume, 0.0f,
                         1.0f)) {
    setMasterVolume(state.audio, state.audio.masterVolume);
  }
  ImGui::End();
}

void drawMapEditorUi(AppState& state) {
  // menu to add models
  ImGui::Begin("Models");
  if (ImGui::Button("Tree")) {
    addModelInstance(state, state.treeAssetIndex, glm::vec3(0.0f, 0.0f, 0.0f),
                     glm::vec3(0.0f), glm::vec3(state.treeScale));
  }
  if (ImGui::Button("Walter")) {
    addModelInstance(state, state.walterAssetIndex, glm::vec3(0.0f, 0.0f, 0.0f),
                     glm::vec3(0.0f), glm::vec3(state.walterScale));
  }
  if (ImGui::Button("Church")) {
    addModelInstance(state, state.churchAssetIndex, glm::vec3(0.0f, 0.0f, 0.0f),
                     glm::vec3(0.0f), glm::vec3(state.churchScale));
  }
  if (ImGui::Button("Flashlight")) {
    addModelInstance(state, state.flashlightAssetIndex,
                     glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f),
                     glm::vec3(state.flashlightScale));
  }
  if (ImGui::Button("Dead Tree")) {
    addModelInstance(state, state.deadtreeAssetIndex,
                     glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f),
                     glm::vec3(state.deadtreeScale));
  }

  ImGui::End();
}

void endDebugUiFrame() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void shutdownDebugUi() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}
