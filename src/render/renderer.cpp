#define GL_SILENCE_DEPRECATION
// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on
#include <stb_image.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "app.hpp"
#include "game/world.hpp"
#include "primitives.hpp"
#include "shader.hpp"

static unsigned int loadCubemap(const std::vector<std::string>& faces);

bool renderInit(AppState& state, GLFWwindow* window) {
  (void)window;
  // glad: load all OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return false;
  }

  // culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  glEnable(GL_DEPTH_TEST);

  // build and compile shader programs
  state.worldShader = new Shader("Shader/default.vs", "Shader/default.fs");
  state.skyboxShader = new Shader("Shader/skybox.vs", "Shader/skybox.fs");

  // regular buffers
  glGenVertexArrays(1, &state.VAO);
  glGenBuffers(1, &state.VBO);
  glGenBuffers(1, &state.EBO);
  glGenBuffers(1, &state.instancedVBO);
  glBindVertexArray(state.VAO);

  // cube geometry
  glBindBuffer(GL_ARRAY_BUFFER, state.VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(CubeVertices), CubeVertices,
               GL_STATIC_DRAW);

  // position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // normal attribute
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(2);

  // texture coord attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(1);

  buildFloor(state);

  glBindBuffer(GL_ARRAY_BUFFER, state.instancedVBO);
  glBufferData(GL_ARRAY_BUFFER,
               state.modelMatrices.size() * sizeof(glm::mat4),
               state.modelMatrices.data(), GL_STATIC_DRAW);

  // Mat4 takes up 4 attribute slots (e.g., locations 3, 4, 5, and 6)
  for (int i = 0; i < 4; i++) {
    glEnableVertexAttribArray(3 + i);
    glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (void*)(sizeof(glm::vec4) * i));

    // Tell OpenGL this is per-instance data, not per-vertex
    glVertexAttribDivisor(3 + i, 1);
  }

  glBindVertexArray(0);

  // skybox
  glGenVertexArrays(1, &state.skyboxVAO);
  glGenBuffers(1, &state.skyboxVBO);
  glBindVertexArray(state.skyboxVAO);
  glBindBuffer(GL_ARRAY_BUFFER, state.skyboxVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

  // Load skybox textures
  std::vector<std::string> faces{
      "assets/skybox/right.jpg", "assets/skybox/left.jpg",
      "assets/skybox/top.jpg",   "assets/skybox/bottom.jpg",
      "assets/skybox/front.jpg", "assets/skybox/back.jpg"};
  state.cubemapTexture = loadCubemap(faces);

  // load and create cube texture
  glGenTextures(1, &state.texture);
  glBindTexture(GL_TEXTURE_2D, state.texture);
  // set the texture wrapping parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // set texture filtering parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // load image, create texture and generate mipmaps
  int width, height, nrChannels;
  unsigned char* data =
      stbi_load("assets/grass.png", &width, &height, &nrChannels, 0);
  if (data) {
    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    std::cout << "Failed to load texture" << std::endl;
  }
  stbi_image_free(data);

  return true;
}

void renderFrame(AppState& state, GLFWwindow* window) {
  glClearColor(state.fogColor.x, state.fogColor.y, state.fogColor.z, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Bind Texture
  glBindTexture(GL_TEXTURE_2D, state.texture);

  // Matrices
  // global space
  glm::mat4 model = glm::mat4(1.0f);

  // view matrix
  glm::mat4 view = state.camera.GetViewMatrix();

  // projection matrix
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  // Ensure we don't divide by zero if window is minimized
  float aspect = (height > 0) ? (float)width / (float)height : 1.0f;

  // Use the dynamic aspect ratio
  glm::mat4 projection =
      glm::perspective(glm::radians(60.0f), aspect, 0.1f,
                       state.renderDistance);

  state.worldShader->use();

  // moon / general light
  glm::vec3 moonDirNorm = glm::normalize(state.moonDir);
  glUniform3fv(glGetUniformLocation(state.worldShader->ID, "viewPos"), 1,
               glm::value_ptr(state.camera.cameraPos));
  glUniform3fv(glGetUniformLocation(state.worldShader->ID, "lightDir"), 1,
               glm::value_ptr(moonDirNorm));
  glUniform3fv(glGetUniformLocation(state.worldShader->ID, "lightColor"), 1,
               glm::value_ptr(state.moonColor));
  glUniform1f(glGetUniformLocation(state.worldShader->ID, "ambientStrength"),
              state.ambientStrength);
  glUniform1f(glGetUniformLocation(state.worldShader->ID, "diffuseStrength"),
              state.diffuseStrength);
  glUniform1f(glGetUniformLocation(state.worldShader->ID, "specularStrength"),
              state.specularStrength);
  glUniform1f(glGetUniformLocation(state.worldShader->ID, "shininess"),
              state.shininess);

  // flashlight spotlight
  glm::vec3 flashlightPos = state.camera.cameraPos;
  glm::vec3 flashlightDir = glm::normalize(state.camera.cameraFront);
  float flashlightInnerCutoff =
      glm::cos(glm::radians(state.flashlightRadius * 0.85f));
  float flashlightOuterCutoff =
      glm::cos(glm::radians(state.flashlightRadius));
  glUniform3fv(glGetUniformLocation(state.worldShader->ID, "spotPos"), 1,
               glm::value_ptr(flashlightPos));
  glUniform3fv(glGetUniformLocation(state.worldShader->ID, "spotDir"), 1,
               glm::value_ptr(flashlightDir));
  glUniform3fv(glGetUniformLocation(state.worldShader->ID, "spotColor"), 1,
               glm::value_ptr(state.flashlightColor));
  float spotIntensity =  // for flashlight toggle
      state.camera.flashlightEnabled ? state.flashlightBrightness : 0.0f;
  glUniform1f(glGetUniformLocation(state.worldShader->ID, "spotIntensity"),
              spotIntensity);
  glUniform1f(glGetUniformLocation(state.worldShader->ID, "spotInnerCutoff"),
              flashlightInnerCutoff);
  glUniform1f(glGetUniformLocation(state.worldShader->ID, "spotOuterCutoff"),
              flashlightOuterCutoff);
  glUniform3fv(glGetUniformLocation(state.worldShader->ID, "fogColor"), 1,
               glm::value_ptr(state.fogColor));
  glUniform1f(glGetUniformLocation(state.worldShader->ID, "fogDensity"),
              state.fogDensity);

  int modelLoc = glGetUniformLocation(state.worldShader->ID, "model");
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
  int viewLoc = glGetUniformLocation(state.worldShader->ID, "view");
  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
  int projectionLoc =
      glGetUniformLocation(state.worldShader->ID, "projection");
  glUniformMatrix4fv(projectionLoc, 1, GL_FALSE,
                     glm::value_ptr(projection));

  // render container
  glPolygonMode(GL_FRONT_AND_BACK, state.wireframe ? GL_LINE : GL_FILL);
  glBindVertexArray(state.VAO);
  glDrawArraysInstanced(GL_TRIANGLES, 0, 36, state.modelMatrices.size());

  // Skybox
  glDepthFunc(GL_LEQUAL);  // disable depth buffer (skybox is at depth 1.0)
  state.skyboxShader->use();

  // Remove translation from view matrix so skybox stays centered on player
  glm::mat4 skyView = glm::mat4(glm::mat3(state.camera.GetViewMatrix()));

  glUniformMatrix4fv(glGetUniformLocation(state.skyboxShader->ID, "view"), 1,
                     GL_FALSE, glm::value_ptr(skyView));
  glUniformMatrix4fv(
      glGetUniformLocation(state.skyboxShader->ID, "projection"), 1, GL_FALSE,
      glm::value_ptr(projection));

  glBindVertexArray(state.skyboxVAO);
  glBindTexture(GL_TEXTURE_CUBE_MAP, state.cubemapTexture);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glDepthFunc(GL_LESS);  // Reset
}

void renderShutdown(AppState& state) {
  if (state.VAO) glDeleteVertexArrays(1, &state.VAO);
  if (state.skyboxVAO) glDeleteVertexArrays(1, &state.skyboxVAO);
  if (state.VBO) glDeleteBuffers(1, &state.VBO);
  if (state.EBO) glDeleteBuffers(1, &state.EBO);
  if (state.instancedVBO) glDeleteBuffers(1, &state.instancedVBO);
  if (state.skyboxVBO) glDeleteBuffers(1, &state.skyboxVBO);
  if (state.texture) glDeleteTextures(1, &state.texture);
  if (state.cubemapTexture) glDeleteTextures(1, &state.cubemapTexture);

  delete state.worldShader;
  state.worldShader = nullptr;
  delete state.skyboxShader;
  state.skyboxShader = nullptr;
}

static unsigned int loadCubemap(const std::vector<std::string>& faces) {
  unsigned int textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

  int width, height, nrChannels;
  for (unsigned int i = 0; i < faces.size(); i++) {
    unsigned char* data =
        stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
    if (data) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height,
                   0, GL_RGB, GL_UNSIGNED_BYTE, data);
      stbi_image_free(data);
    } else {
      std::cout << "Cubemap tex failed to load at path: " << faces[i]
                << std::endl;
      stbi_image_free(data);
    }
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  return textureID;
}
