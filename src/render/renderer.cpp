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
#include "primitives.hpp"
#include "render/shader.hpp"
#include "scene/world.hpp"

static unsigned int loadCubemap(const std::vector<std::string>& faces);
static void uploadTerrainBuffers(AppState& state);
static void uploadGrassInstances(AppState& state);
static void uploadTreeInstances(AppState& state);
static void ensureRenderTarget(AppState& state, int framebufferWidth,
                               int framebufferHeight);

bool renderInit(AppState& state, GLFWwindow* window) {
  // glad: load all OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return false;
  }

  // culling
  // glEnable(GL_CULL_FACE);
  // glCullFace(GL_BACK);
  // glFrontFace(GL_CCW);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  int framebufferWidth = 0;
  int framebufferHeight = 0;
  glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
  ensureRenderTarget(state, framebufferWidth, framebufferHeight);

  // build and compile shader programs
  state.worldShader = new Shader("Shader/default.vs", "Shader/default.fs");
  state.skyboxShader = new Shader("Shader/skybox.vs", "Shader/skybox.fs");
  state.grassShader = new Shader("Shader/grass.vs", "Shader/grass.fs");

  // regular buffers
  glGenVertexArrays(1, &state.VAO);
  glGenBuffers(1, &state.VBO);
  glGenBuffers(1, &state.EBO);
  glBindVertexArray(state.VAO);

  buildFloor(state);
  uploadTerrainBuffers(state);
  state.terrainDirty = false;

  glBindVertexArray(state.VAO);

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

  glBindVertexArray(0);

  // grass geometry
  float grassWidth = state.cubeScale * 0.6f;
  float grassHeight = state.cubeScale * 0.9f;
  float halfWidth = grassWidth * 0.5f;
  float grassVertices[] = {
      -halfWidth, 0.0f,        0.0f,       0.0f, 0.0f,  // quad A
      halfWidth,  0.0f,        0.0f,       1.0f, 0.0f,
      halfWidth,  grassHeight, 0.0f,       1.0f, 1.0f,
      -halfWidth, grassHeight, 0.0f,       0.0f, 1.0f,
      0.0f,       0.0f,        -halfWidth, 0.0f, 0.0f,  // quad B
      0.0f,       0.0f,        halfWidth,  1.0f, 0.0f,
      0.0f,       grassHeight, halfWidth,  1.0f, 1.0f,
      0.0f,       grassHeight, -halfWidth, 0.0f, 1.0f};
  unsigned int grassIndices[] = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};
  state.grassIndexCount =
      static_cast<int>(sizeof(grassIndices) / sizeof(grassIndices[0]));

  glGenVertexArrays(1, &state.grassVAO);
  glGenBuffers(1, &state.grassVBO);
  glGenBuffers(1, &state.grassEBO);
  glGenBuffers(1, &state.grassInstanceVBO);

  glBindVertexArray(state.grassVAO);
  glBindBuffer(GL_ARRAY_BUFFER, state.grassVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(grassVertices), grassVertices,
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.grassEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(grassIndices), grassIndices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, state.grassInstanceVBO);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
  glEnableVertexAttribArray(3);
  glVertexAttribDivisor(3, 1);
  glBindVertexArray(0);

  buildGrass(state);
  uploadGrassInstances(state);
  state.grassDirty = false;

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

  // load grass texture
  glGenTextures(1, &state.grassTexture);
  glBindTexture(GL_TEXTURE_2D, state.grassTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  int grassWidthPx, grassHeightPx, grassChannels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char* grassData =
      stbi_load("assets/environment/grass_blades.png", &grassWidthPx,
                &grassHeightPx, &grassChannels, 0);
  stbi_set_flip_vertically_on_load(false);
  if (grassData) {
    GLenum format = (grassChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, grassWidthPx, grassHeightPx, 0,
                 format, GL_UNSIGNED_BYTE, grassData);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    std::cout << "Failed to load grass texture" << std::endl;
  }
  stbi_image_free(grassData);

  return true;
}

void renderFrame(AppState& state, GLFWwindow* window) {
  int framebufferWidth = 0;
  int framebufferHeight = 0;
  glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
  ensureRenderTarget(state, framebufferWidth, framebufferHeight);
  glBindFramebuffer(GL_FRAMEBUFFER, state.renderTargetFbo);
  glViewport(0, 0, state.renderTargetWidth, state.renderTargetHeight);

  if (state.terrainDirty) {
    buildFloor(state);
    uploadTerrainBuffers(state);
    updateWorldModelHeights(state);
    state.terrainDirty = false;
    state.grassDirty = true;
    state.treeInstanceDirty = true;
  }
  if (state.treeDirty) {
    rebuildWorldTrees(state);
    state.treeDirty = false;
    state.treeInstanceDirty = true;
  }

  float grassUpdateDistance = state.cubeScale * 2.0f;
  float grassDx = state.camera.cameraPos.x - state.grassCenter.x;
  float grassDz = state.camera.cameraPos.z - state.grassCenter.y;
  float grassMoveSq = (grassDx * grassDx) + (grassDz * grassDz);
  float grassUpdateSq = grassUpdateDistance * grassUpdateDistance;
  if (grassMoveSq > grassUpdateSq) {
    state.grassDirty = true;
  }

  if (state.grassDirty) {
    buildGrass(state);
    uploadGrassInstances(state);
    state.grassDirty = false;
  }
  if (state.treeUpdateDistance > 0.0f) {
    float treeMoveDx = state.camera.cameraPos.x - state.treeCullCenter.x;
    float treeMoveDz = state.camera.cameraPos.z - state.treeCullCenter.y;
    float treeMoveSq = (treeMoveDx * treeMoveDx) + (treeMoveDz * treeMoveDz);
    float treeUpdateSq = state.treeUpdateDistance * state.treeUpdateDistance;
    if (treeMoveSq > treeUpdateSq) {
      state.treeInstanceDirty = true;
    }
  }
  if (state.treeInstanceVBO == 0 && state.treeAssetIndex >= 0) {
    state.treeInstanceDirty = true;
  }
  if (state.treeInstanceDirty) {
    uploadTreeInstances(state);
  }

  glClearColor(state.fogColor.x, state.fogColor.y, state.fogColor.z, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Bind Texture
  glBindTexture(GL_TEXTURE_2D, state.texture);

  // Matrices
  glm::mat4 model = glm::mat4(1.0f);
  glm::mat4 view = state.camera.GetViewMatrix();
  int renderWidth = state.renderTargetWidth;
  int renderHeight = state.renderTargetHeight;
  float aspect =
      (renderHeight > 0) ? (float)renderWidth / (float)renderHeight : 1.0f;
  glm::mat4 projection =
      glm::perspective(glm::radians(60.0f), aspect, 0.1f, state.renderDistance);

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
  glUniform1i(glGetUniformLocation(state.worldShader->ID, "normalMap"), 0);
  glUniform1i(glGetUniformLocation(state.worldShader->ID, "useNormalMap"), 0);
  glUniform1f(glGetUniformLocation(state.worldShader->ID, "normalStrength"),
              1.0f);
  glUniform1i(glGetUniformLocation(state.worldShader->ID, "normalDebug"), 0);
  glUniform1i(glGetUniformLocation(state.worldShader->ID, "doubleSided"), 0);
  glUniform1i(glGetUniformLocation(state.worldShader->ID, "useInstancing"), 0);
  glUniform1i(glGetUniformLocation(state.worldShader->ID, "depthOnly"), 0);

  // flashlight spotlight
  glm::vec3 flashlightPos = state.camera.cameraPos;
  glm::vec3 flashlightDir = glm::normalize(state.camera.cameraFront);
  float flashlightInnerCutoff =
      glm::cos(glm::radians(state.flashlightRadius * 0.85f));
  float flashlightOuterCutoff = glm::cos(glm::radians(state.flashlightRadius));
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
  glUniform1f(glGetUniformLocation(state.worldShader->ID, "alphaCutoff"), 0.0f);
  glUniform1i(glGetUniformLocation(state.worldShader->ID, "ourTexture"), 0);
  glUniform1f(glGetUniformLocation(state.worldShader->ID, "albedoIntensity"),
              1.0f);

  int modelLoc = glGetUniformLocation(state.worldShader->ID, "model");
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
  int viewLoc = glGetUniformLocation(state.worldShader->ID, "view");
  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
  int projectionLoc = glGetUniformLocation(state.worldShader->ID, "projection");
  glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

  // render container
  glPolygonMode(GL_FRONT_AND_BACK, state.wireframe ? GL_LINE : GL_FILL);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, state.texture);
  glBindVertexArray(state.VAO);
  glDrawElements(GL_TRIANGLES,
                 static_cast<GLsizei>(state.terrainIndices.size()),
                 GL_UNSIGNED_INT, 0);

  if (!state.modelInstances.empty()) {
    bool drewTrees = false;
    if (state.treeAssetIndex >= 0 &&
        state.treeAssetIndex < static_cast<int>(state.modelAssets.size()) &&
        state.treeInstanceCount > 0) {
      ModelAsset& asset = state.modelAssets[state.treeAssetIndex];
      if (asset.model.IsLoaded()) {
        const ModelRenderSettings& settings = asset.renderSettings;
        glUniform1f(
            glGetUniformLocation(state.worldShader->ID, "albedoIntensity"),
            settings.albedoIntensity);
        glUniform1f(glGetUniformLocation(state.worldShader->ID, "alphaCutoff"),
                    settings.alphaCutoff);
        glUniform1f(
            glGetUniformLocation(state.worldShader->ID, "normalStrength"),
            settings.normalStrength);
        glUniform1i(glGetUniformLocation(state.worldShader->ID, "normalDebug"),
                    settings.normalDebug ? 1 : 0);
        glUniform1i(glGetUniformLocation(state.worldShader->ID, "doubleSided"),
                    settings.doubleSided ? 1 : 0);
        glUniform1i(glGetUniformLocation(state.worldShader->ID, "useNormalMap"),
                    settings.useNormalMap ? 1 : 0);
        glUniform1i(glGetUniformLocation(state.worldShader->ID, "normalMap"),
                    settings.useNormalMap ? 1 : 0);
        glUniform1i(
            glGetUniformLocation(state.worldShader->ID, "useInstancing"), 1);
        glUniform1i(glGetUniformLocation(state.worldShader->ID, "depthOnly"),
                    0);
        glUniform1i(glGetUniformLocation(state.worldShader->ID, "useNormalMap"),
                    settings.useNormalMap ? 1 : 0);
        glUniform1i(glGetUniformLocation(state.worldShader->ID, "normalMap"),
                    settings.useNormalMap ? 1 : 0);
        glm::mat4 identity = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                           glm::value_ptr(identity));
        for (const ModelMesh& mesh : asset.model.GetMeshes()) {
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, mesh.texture);
          if (settings.useNormalMap) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, mesh.normalMap);
          }
          glBindVertexArray(mesh.vao);
          glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount,
                                  GL_UNSIGNED_INT, 0,
                                  state.treeInstanceCount);
        }
        drewTrees = true;
      }
    }

    glUniform1i(
        glGetUniformLocation(state.worldShader->ID, "useInstancing"), 0);
    glUniform1i(glGetUniformLocation(state.worldShader->ID, "depthOnly"), 0);

    for (const ModelInstance& instance : state.modelInstances) {
      if (instance.assetIndex < 0 ||
          instance.assetIndex >=
              static_cast<int>(state.modelAssets.size())) {
        continue;
      }
      if (drewTrees && instance.assetIndex == state.treeAssetIndex) {
        continue;
      }

      ModelAsset& asset = state.modelAssets[instance.assetIndex];
      if (!asset.model.IsLoaded()) {
        continue;
      }

      glm::mat4 modelMatrix = glm::mat4(1.0f);
      modelMatrix = glm::translate(modelMatrix, instance.position);
      modelMatrix =
          glm::rotate(modelMatrix, glm::radians(instance.rotation.x),
                      glm::vec3(1.0f, 0.0f, 0.0f));
      modelMatrix =
          glm::rotate(modelMatrix, glm::radians(instance.rotation.y),
                      glm::vec3(0.0f, 1.0f, 0.0f));
      modelMatrix =
          glm::rotate(modelMatrix, glm::radians(instance.rotation.z),
                      glm::vec3(0.0f, 0.0f, 1.0f));
      modelMatrix = glm::scale(modelMatrix, instance.scale);

      const ModelRenderSettings& settings = asset.renderSettings;
      glUniform1f(
          glGetUniformLocation(state.worldShader->ID, "albedoIntensity"),
          settings.albedoIntensity);
      glUniform1f(glGetUniformLocation(state.worldShader->ID, "alphaCutoff"),
                  settings.alphaCutoff);
      glUniform1f(glGetUniformLocation(state.worldShader->ID, "normalStrength"),
                  settings.normalStrength);
      glUniform1i(glGetUniformLocation(state.worldShader->ID, "normalDebug"),
                  settings.normalDebug ? 1 : 0);
      glUniform1i(glGetUniformLocation(state.worldShader->ID, "doubleSided"),
                  settings.doubleSided ? 1 : 0);
      glUniform1i(glGetUniformLocation(state.worldShader->ID, "useNormalMap"),
                  settings.useNormalMap ? 1 : 0);
      glUniform1i(glGetUniformLocation(state.worldShader->ID, "normalMap"),
                  settings.useNormalMap ? 1 : 0);

      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
      for (const ModelMesh& mesh : asset.model.GetMeshes()) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.texture);
        if (settings.useNormalMap) {
          glActiveTexture(GL_TEXTURE1);
          glBindTexture(GL_TEXTURE_2D, mesh.normalMap);
        }
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
      }
    }

    glUniform1f(glGetUniformLocation(state.worldShader->ID, "alphaCutoff"),
                0.0f);
    glUniform1i(glGetUniformLocation(state.worldShader->ID, "useNormalMap"),
                0);
    glUniform1i(glGetUniformLocation(state.worldShader->ID, "normalMap"), 0);
    glUniform1f(glGetUniformLocation(state.worldShader->ID, "normalStrength"),
                1.0f);
    glUniform1i(glGetUniformLocation(state.worldShader->ID, "normalDebug"), 0);
    glUniform1i(glGetUniformLocation(state.worldShader->ID, "doubleSided"), 0);
    glUniform1i(
        glGetUniformLocation(state.worldShader->ID, "useInstancing"), 0);
    glUniform1f(glGetUniformLocation(state.worldShader->ID, "albedoIntensity"),
                1.0f);
  }

  if (!state.grassInstances.empty()) {
    state.grassShader->use();
    glUniform3fv(glGetUniformLocation(state.grassShader->ID, "viewPos"), 1,
                 glm::value_ptr(state.camera.cameraPos));
    glUniform3fv(glGetUniformLocation(state.grassShader->ID, "lightDir"), 1,
                 glm::value_ptr(moonDirNorm));
    glUniform3fv(glGetUniformLocation(state.grassShader->ID, "lightColor"), 1,
                 glm::value_ptr(state.moonColor));
    glUniform1f(glGetUniformLocation(state.grassShader->ID, "ambientStrength"),
                state.ambientStrength);
    glUniform1f(glGetUniformLocation(state.grassShader->ID, "diffuseStrength"),
                state.diffuseStrength);
    glUniform3fv(glGetUniformLocation(state.grassShader->ID, "spotPos"), 1,
                 glm::value_ptr(flashlightPos));
    glUniform3fv(glGetUniformLocation(state.grassShader->ID, "spotDir"), 1,
                 glm::value_ptr(flashlightDir));
    glUniform3fv(glGetUniformLocation(state.grassShader->ID, "spotColor"), 1,
                 glm::value_ptr(state.flashlightColor));
    glUniform1f(glGetUniformLocation(state.grassShader->ID, "spotIntensity"),
                spotIntensity);
    glUniform1f(glGetUniformLocation(state.grassShader->ID, "spotInnerCutoff"),
                flashlightInnerCutoff);
    glUniform1f(glGetUniformLocation(state.grassShader->ID, "spotOuterCutoff"),
                flashlightOuterCutoff);
    glUniform1f(glGetUniformLocation(state.grassShader->ID, "grassIntensity"),
                state.grassIntensity);
    glUniform3fv(glGetUniformLocation(state.grassShader->ID, "fogColor"), 1,
                 glm::value_ptr(state.fogColor));
    glUniform1f(glGetUniformLocation(state.grassShader->ID, "fogDensity"),
                state.fogDensity);
    glUniformMatrix4fv(glGetUniformLocation(state.grassShader->ID, "view"), 1,
                       GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(
        glGetUniformLocation(state.grassShader->ID, "projection"), 1, GL_FALSE,
        glm::value_ptr(projection));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, state.grassTexture);
    glUniform1i(glGetUniformLocation(state.grassShader->ID, "grassTexture"), 0);
    glBindVertexArray(state.grassVAO);
    glDrawElementsInstanced(GL_TRIANGLES, state.grassIndexCount,
                            GL_UNSIGNED_INT, 0,
                            static_cast<GLsizei>(state.grassInstances.size()));
  }

  // Skybox
  glDepthFunc(GL_LEQUAL);  // disable depth buffer (skybox is at depth 1.0)
  state.skyboxShader->use();

  // Remove translation from view matrix so skybox stays centered on player
  glm::mat4 skyView = glm::mat4(glm::mat3(state.camera.GetViewMatrix()));

  glUniformMatrix4fv(glGetUniformLocation(state.skyboxShader->ID, "view"), 1,
                     GL_FALSE, glm::value_ptr(skyView));
  glUniformMatrix4fv(glGetUniformLocation(state.skyboxShader->ID, "projection"),
                     1, GL_FALSE, glm::value_ptr(projection));
  glUniform1f(glGetUniformLocation(state.skyboxShader->ID, "skyboxIntensity"),
              state.skyboxIntensity);

  glBindVertexArray(state.skyboxVAO);
  glBindTexture(GL_TEXTURE_CUBE_MAP, state.cubemapTexture);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glDepthFunc(GL_LESS);  // Reset

  glBindFramebuffer(GL_READ_FRAMEBUFFER, state.renderTargetFbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, state.renderTargetWidth, state.renderTargetHeight, 0,
                    0, framebufferWidth, framebufferHeight,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, framebufferWidth, framebufferHeight);
}

void renderShutdown(AppState& state) {
  if (state.VAO) glDeleteVertexArrays(1, &state.VAO);
  if (state.skyboxVAO) glDeleteVertexArrays(1, &state.skyboxVAO);
  if (state.VBO) glDeleteBuffers(1, &state.VBO);
  if (state.EBO) glDeleteBuffers(1, &state.EBO);
  if (state.skyboxVBO) glDeleteBuffers(1, &state.skyboxVBO);
  if (state.texture) glDeleteTextures(1, &state.texture);
  if (state.cubemapTexture) glDeleteTextures(1, &state.cubemapTexture);
  if (state.grassTexture) glDeleteTextures(1, &state.grassTexture);
  if (state.grassVAO) glDeleteVertexArrays(1, &state.grassVAO);
  if (state.grassVBO) glDeleteBuffers(1, &state.grassVBO);
  if (state.grassEBO) glDeleteBuffers(1, &state.grassEBO);
  if (state.grassInstanceVBO) glDeleteBuffers(1, &state.grassInstanceVBO);
  if (state.treeInstanceVBO) glDeleteBuffers(1, &state.treeInstanceVBO);
  if (state.renderTargetColor) glDeleteTextures(1, &state.renderTargetColor);
  if (state.renderTargetDepth)
    glDeleteRenderbuffers(1, &state.renderTargetDepth);
  if (state.renderTargetFbo) glDeleteFramebuffers(1, &state.renderTargetFbo);
  for (ModelAsset& asset : state.modelAssets) {
    asset.model.Shutdown();
  }
  state.modelAssets.clear();
  state.modelInstances.clear();
  state.treeAssetIndex = -1;
  state.treeInstanceIndex = -1;

  delete state.worldShader;
  state.worldShader = nullptr;
  delete state.skyboxShader;
  state.skyboxShader = nullptr;
  delete state.grassShader;
  state.grassShader = nullptr;
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

static void uploadTerrainBuffers(AppState& state) {
  glBindVertexArray(state.VAO);
  glBindBuffer(GL_ARRAY_BUFFER, state.VBO);
  glBufferData(GL_ARRAY_BUFFER, state.terrainVertices.size() * sizeof(float),
               state.terrainVertices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               state.terrainIndices.size() * sizeof(unsigned int),
               state.terrainIndices.data(), GL_STATIC_DRAW);
  glBindVertexArray(0);
}

static void uploadGrassInstances(AppState& state) {
  glBindVertexArray(state.grassVAO);
  glBindBuffer(GL_ARRAY_BUFFER, state.grassInstanceVBO);
  glBufferData(GL_ARRAY_BUFFER, state.grassInstances.size() * sizeof(glm::vec3),
               state.grassInstances.data(), GL_STATIC_DRAW);
  glBindVertexArray(0);
}

static void uploadTreeInstances(AppState& state) {
  state.treeInstanceCount = 0;
  if (state.treeAssetIndex < 0 ||
      state.treeAssetIndex >= static_cast<int>(state.modelAssets.size())) {
    state.treeInstanceDirty = false;
    return;
  }

  ModelAsset& asset = state.modelAssets[state.treeAssetIndex];
  if (!asset.model.IsLoaded()) {
    state.treeInstanceDirty = false;
    return;
  }

  std::vector<glm::mat4> matrices;
  matrices.reserve(state.modelInstances.size());
  state.treeCollisionPositions.clear();
  state.treeCollisionPositions.reserve(state.modelInstances.size());
  float radius = state.treeRenderRadius;
  if (radius < 0.0f) {
    radius = 0.0f;
  }
  float radiusSq = radius * radius;
  float centerX = state.camera.cameraPos.x;
  float centerZ = state.camera.cameraPos.z;
  for (const ModelInstance& instance : state.modelInstances) {
    if (instance.assetIndex != state.treeAssetIndex) {
      continue;
    }
    float dx = instance.position.x - centerX;
    float dz = instance.position.z - centerZ;
    if ((dx * dx + dz * dz) > radiusSq) {
      continue;
    }

    state.treeCollisionPositions.push_back(instance.position);

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, instance.position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(instance.rotation.x),
                              glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(instance.rotation.y),
                              glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(instance.rotation.z),
                              glm::vec3(0.0f, 0.0f, 1.0f));
    modelMatrix = glm::scale(modelMatrix, instance.scale);
    matrices.push_back(modelMatrix);
  }

  state.treeInstanceCount = static_cast<int>(matrices.size());
  if (state.treeInstanceCount == 0) {
    state.treeCollisionPositions.clear();
    state.treeInstanceDirty = false;
    return;
  }

  if (state.treeInstanceVBO == 0) {
    glGenBuffers(1, &state.treeInstanceVBO);
  }

  glBindBuffer(GL_ARRAY_BUFFER, state.treeInstanceVBO);
  glBufferData(GL_ARRAY_BUFFER, matrices.size() * sizeof(glm::mat4),
               matrices.data(), GL_STATIC_DRAW);

  std::size_t vec4Size = sizeof(glm::vec4);
  for (const ModelMesh& mesh : asset.model.GetMeshes()) {
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, state.treeInstanceVBO);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (void*)0);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (void*)(vec4Size));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (void*)(vec4Size * 2));
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (void*)(vec4Size * 3));
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
  }

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  state.treeCullCenter = glm::vec2(centerX, centerZ);
  state.treeInstanceDirty = false;
}

static void ensureRenderTarget(AppState& state, int framebufferWidth,
                               int framebufferHeight) {
  float scale = state.renderScale;
  if (scale <= 0.0f) {
    scale = 1.0f;
  }
  int targetWidth = static_cast<int>(framebufferWidth * scale);
  int targetHeight = static_cast<int>(framebufferHeight * scale);
  if (targetWidth < 1) {
    targetWidth = 1;
  }
  if (targetHeight < 1) {
    targetHeight = 1;
  }
  if (state.renderTargetFbo != 0 &&
      targetWidth == state.renderTargetWidth &&
      targetHeight == state.renderTargetHeight) {
    return;
  }

  if (state.renderTargetColor) {
    glDeleteTextures(1, &state.renderTargetColor);
    state.renderTargetColor = 0;
  }
  if (state.renderTargetDepth) {
    glDeleteRenderbuffers(1, &state.renderTargetDepth);
    state.renderTargetDepth = 0;
  }
  if (state.renderTargetFbo) {
    glDeleteFramebuffers(1, &state.renderTargetFbo);
    state.renderTargetFbo = 0;
  }

  glGenFramebuffers(1, &state.renderTargetFbo);
  glBindFramebuffer(GL_FRAMEBUFFER, state.renderTargetFbo);

  glGenTextures(1, &state.renderTargetColor);
  glBindTexture(GL_TEXTURE_2D, state.renderTargetColor);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, targetWidth, targetHeight, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         state.renderTargetColor, 0);

  glGenRenderbuffers(1, &state.renderTargetDepth);
  glBindRenderbuffer(GL_RENDERBUFFER, state.renderTargetDepth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, targetWidth,
                        targetHeight);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, state.renderTargetDepth);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "Render target incomplete" << std::endl;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  state.renderTargetWidth = targetWidth;
  state.renderTargetHeight = targetHeight;
}
