#include "ui/debugUi.hpp"

#include <GLFW/glfw3.h>

#include "app.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

bool initDebugUi(GLFWwindow* window) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
  return true;
}

void beginDebugUiFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void drawDebugUi(AppState& state) {
  ImGuiIO& io = ImGui::GetIO();

  ImGui::Begin("FPS");
  ImGui::Text("%.f", io.Framerate);
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
  ImGui::SliderFloat("Flashlight Brightness", &state.flashlightBrightness,
                     0.0f, 10.0f);
  ImGui::SliderFloat("Flashlight Radius (deg)", &state.flashlightRadius, 1.0f,
                     60.0f);
  ImGui::SliderFloat("Fog", &state.fogDensity, 0.005f, 0.50f);
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
