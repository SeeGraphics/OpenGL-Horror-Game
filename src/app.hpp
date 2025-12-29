#ifndef APP_HPP
#define APP_HPP

#include <vector>

#include <glm/glm.hpp>

#include "audio/audio.hpp"
#include "scene/camera.hpp"

class Shader;
struct GLFWwindow;

struct AppState {
  float cubeScale = 1.0f;

  float deltaTime = 0.0f;
  float lastFrame = 0.0f;

  int floorSize = 100;
  float floorY = -1.0f;

  float renderDistance = 500.0f;

  float ambientStrength = 0.05f;
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

  bool fullscreen = true;
  bool wireframe = false;
  bool freeCam = false;

  Camera camera;
  AudioSystem audio;

  Shader* worldShader = nullptr;
  Shader* skyboxShader = nullptr;

  unsigned int VBO = 0;
  unsigned int VAO = 0;
  unsigned int EBO = 0;
  unsigned int skyboxVAO = 0;
  unsigned int skyboxVBO = 0;
  unsigned int cubemapTexture = 0;
  unsigned int texture = 0;

  std::vector<float> terrainVertices;
  std::vector<unsigned int> terrainIndices;
};

bool AppInit(AppState& state, GLFWwindow* window);
void AppFrame(AppState& state, GLFWwindow* window);
void AppShutdown(AppState& state);

#endif
