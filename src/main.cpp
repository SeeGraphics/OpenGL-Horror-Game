#define GL_SILENCE_DEPRECATION
// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "camera.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "primitives.hpp"
#include "shader.hpp"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
unsigned int loadCubemap(std::vector<std::string> faces);

// settings
// commented out since we use the fullscreen on startup
// const unsigned int SCR_WIDTH = 1200;
// const unsigned int SCR_HEIGHT = 800;
float cubeScale = 1.0f;  // for minecraft size cubes

float deltaTime = 0.0f;  // Time between current frame and last frame
float lastFrame = 0.0f;  // Time of last frame

int floorsize = 100;
float floorY = -1.0f;

float renderDistance = 500.0f;

// toggle vars
bool fullscreen = true;
bool wireframe = false;
bool freeCam = false;

// camera
Camera camera;

int main() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // for mac
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);

  // glfw window creation fullscreen on startup
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  GLFWwindow* window = glfwCreateWindow(mode->width, mode->height,
                                        "OpenGL Window", monitor, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSwapInterval(0);  // no v-sync

  // glad: load all OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // imgui load
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  // build and compile shader programs
  Shader ourShader("Shader/default.vs", "Shader/default.fs");
  Shader skyboxShader("Shader/skybox.vs", "Shader/skybox.fs");

  // regular buffers
  unsigned int VBO, VAO, EBO, instancedVBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glGenBuffers(1, &instancedVBO);
  glBindVertexArray(VAO);

  // cube geometry
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(CubeVertices), CubeVertices,
               GL_STATIC_DRAW);

  // position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // texture coord attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  std::vector<glm::mat4> modelMatrices;
  for (int x = -floorsize; x < floorsize; x++) {
    for (int z = -floorsize; z < floorsize; z++) {
      glm::mat4 model = glm::mat4(1.0f);
      float xPos = (float)x * cubeScale;
      float zPos = (float)z * cubeScale;
      model = glm::translate(model,
                             glm::vec3(xPos, floorY, zPos));  // floor is at -1
      model = glm::scale(model, glm::vec3(cubeScale));
      modelMatrices.push_back(model);
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, instancedVBO);
  glBufferData(GL_ARRAY_BUFFER, modelMatrices.size() * sizeof(glm::mat4),
               &modelMatrices[0], GL_STATIC_DRAW);

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
  unsigned int skyboxVAO, skyboxVBO;
  glGenVertexArrays(1, &skyboxVAO);
  glGenBuffers(1, &skyboxVBO);
  glBindVertexArray(skyboxVAO);
  glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

  // Load skybox textures
  std::vector<std::string> faces{
      "assets/skybox/right.jpg", "assets/skybox/left.jpg",
      "assets/skybox/top.jpg",   "assets/skybox/bottom.jpg",
      "assets/skybox/front.jpg", "assets/skybox/back.jpg"};
  unsigned int cubemapTexture = loadCubemap(faces);

  // load and create cube texture
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
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

  // render loop
  while (!glfwWindowShouldClose(window)) {
    // calculate delta time
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // input
    glfwSetCursorPosCallback(window, mouse_callback);
    processInput(window);

    // render

    // imgui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("FPS");
    ImGui::Text("%.f", io.Framerate);
    ImGui::End();

    ImGui::Begin("Settings");
    ImGui::Checkbox("Free Cam", &freeCam);
    ImGui::Checkbox("Wireframe", &wireframe);
    ImGui::PushItemWidth(50);
    ImGui::SliderFloat("Render Distance", &renderDistance, 5.0f, 1000.0f);
    ImGui::PopItemWidth();
    ImGui::End();

    ImGui::Begin("Environment");
    ImGui::End();

    // OPEN_GL
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind Texture
    glBindTexture(GL_TEXTURE_2D, texture);

    // Matrices
    // global space
    glm::mat4 model = glm::mat4(1.0f);

    // apply gravity & floor collision (unless freeCam is on)
    if (!freeCam) {
      if (!camera.isGrounded) {
        camera.velocity.y += camera.GRAVITY * deltaTime;
      }

      camera.cameraPos += camera.velocity * deltaTime;
      float floorLevel = floorY + camera.cameraHeight;

      if (camera.cameraPos.y <= floorLevel) {
        camera.cameraPos.y = floorLevel;  // Snap to floor
        camera.velocity.y = 0.0f;         // Stop falling
        camera.isGrounded = true;
      } else {
        camera.isGrounded = false;
      }
    }

    // view matrix
    glm::mat4 view = camera.GetViewMatrix();

    // projection matrix
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    // Ensure we don't divide by zero if window is minimized
    float aspect = (height > 0) ? (float)width / (float)height : 1.0f;

    // Use the dynamic aspect ratio
    glm::mat4 projection =
        glm::perspective(glm::radians(60.0f), aspect, 0.1f, renderDistance);

    ourShader.use();
    int modelLoc = glGetUniformLocation(ourShader.ID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    int viewLoc = glGetUniformLocation(ourShader.ID, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    int projectionLoc = glGetUniformLocation(ourShader.ID, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // render container
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 36, modelMatrices.size());

    // Skybox
    glDepthFunc(GL_LEQUAL);  // disable depth buffer (skybox is at depth 1.0)
    skyboxShader.use();

    // Remove translation from view matrix so skybox stays centered on player
    glm::mat4 skyView = glm::mat4(glm::mat3(camera.GetViewMatrix()));

    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.ID, "view"), 1,
                       GL_FALSE, glm::value_ptr(skyView));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.ID, "projection"), 1,
                       GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(skyboxVAO);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthFunc(GL_LESS);  // Reset

    // render imgui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // glfw: swap buffers and poll IO events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // de-allocate all resources once theyve outlived their purpose:
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);

  // imgui: terminate
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  // glfw: terminate, clearing all previously allocated GLFW resources.
  glfwTerminate();
  return 0;
}

void processInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
  // toggle freeCam
  static bool vWasDown = false;
  if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
    if (!vWasDown) {
      freeCam = !freeCam;  // Toggle only once per press
    }
    vWasDown = true;
  } else {
    vWasDown = false;
  }
  if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
    if (!fullscreen) {
      // Switch to fullscreen
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height,
                           mode->refreshRate);
      fullscreen = true;
    } else {
      // Switch back to windowed
      glfwSetWindowMonitor(window, NULL, 100, 100, 800, 600, 0);
      fullscreen = false;
    }
  }
  // player/ camera controls from camera.cpp
  camera.ProcessKeyboard(window, deltaTime, freeCam);
  static bool pWasDown = false;

  // toggle Wireframe mode
  if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
    if (!pWasDown) {
      wireframe = !wireframe;
    }
    pWasDown = true;
  } else {
    pWasDown = false;
  }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
  (void)window;
  if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
    camera.ProcessMouse(xpos, ypos);
  }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  (void)window;
  glViewport(0, 0, width, height);
}

// for skybox
unsigned int loadCubemap(std::vector<std::string> faces) {
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
