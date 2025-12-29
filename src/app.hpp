#ifndef APP_HPP
#define APP_HPP

#include <glm/glm.hpp>
#include <vector>

#include "audio/audio.hpp"
#include "render/model.hpp"
#include "scene/camera.hpp"

class Shader;
struct GLFWwindow;

struct AppState {
  float cubeScale = 1.0f;

  float deltaTime = 0.0f;
  float lastFrame = 0.0f;

  int floorSize = 500;
  float floorY = -1.0f;

  float renderDistance = 1000.0f;

  float ambientStrength = 1.55f;  // dark: 0.05f, increased for testing
  float diffuseStrength = 0.35f;
  float specularStrength = 0.010f;  // get rid of those circles on the floor
  float shininess = 32.0f;
  glm::vec3 moonDir = glm::vec3(-0.2f, -1.0f, -0.3f);
  glm::vec3 moonColor = glm::vec3(0.6f, 0.65f, 0.8f);
  float flashlightBrightness = 3.5f;
  float flashlightRadius = 28.0f;
  glm::vec3 flashlightColor = glm::vec3(1.0f, 0.95f, 0.85f);
  float fogDensity = 0.02f;
  glm::vec3 fogColor = glm::vec3(0.02f, 0.02f, 0.03f);
  float heightmapScale = 40.0f;
  bool terrainDirty = true;
  float grassDensity = 0.05f;
  float grassIntensity = 0.4f;
  float grassNearRadius = 35.0f;
  float grassFarRadius = 90.0f;
  float grassMidDensity = 0.35f;
  bool grassDirty = true;
  glm::vec2 grassCenter = glm::vec2(0.0f);
  float treeDensity = 0.02f;
  bool treeDirty = false;
  bool treeInstanceDirty = false;
  float treeRenderRadius = 80.0f;
  float treeUpdateDistance = 6.0f;
  glm::vec2 treeCullCenter = glm::vec2(0.0f);
  float skyboxIntensity = 0.55f;  // dark: 0.05f, increased for testing

  bool fullscreen = true;
  bool wireframe = false;
  bool freeCam = false;
  bool showDebugUi = true;

  Camera camera;
  AudioSystem audio;

  Shader* worldShader = nullptr;
  Shader* skyboxShader = nullptr;
  Shader* grassShader = nullptr;
  std::vector<ModelAsset> modelAssets;
  std::vector<ModelInstance> modelInstances;
  int treeAssetIndex = -1;
  int treeInstanceIndex = -1;

  unsigned int VBO = 0;
  unsigned int VAO = 0;
  unsigned int EBO = 0;
  unsigned int skyboxVAO = 0;
  unsigned int skyboxVBO = 0;
  unsigned int cubemapTexture = 0;
  unsigned int texture = 0;
  unsigned int grassVAO = 0;
  unsigned int grassVBO = 0;
  unsigned int grassEBO = 0;
  unsigned int grassInstanceVBO = 0;
  unsigned int grassTexture = 0;
  int grassIndexCount = 0;
  unsigned int treeInstanceVBO = 0;
  int treeInstanceCount = 0;
  std::vector<glm::vec3> treeCollisionPositions;

  std::vector<float> terrainVertices;
  std::vector<unsigned int> terrainIndices;
  std::vector<glm::vec3> grassInstances;
};

bool AppInit(AppState& state, GLFWwindow* window);
void AppFrame(AppState& state, GLFWwindow* window);
void AppShutdown(AppState& state);

#endif
