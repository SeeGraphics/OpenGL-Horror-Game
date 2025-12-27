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

// settings
// commented out since we use the fullscreen on startupq
// const unsigned int SCR_WIDTH = 1200;
// const unsigned int SCR_HEIGHT = 800;
float cubeScale = 1.0f;  // for minecraft size cubes

float deltaTime = 0.0f;  // Time between current frame and last frame
float lastFrame = 0.0f;  // Time of last frame

int floorsize = 100;
float floorY = -1.0f;

// toggle vars
bool fullscreen = true;
bool wireframe = false;

// camera
// ---------
Camera camera;

int main() {
  // glfw: initialize and configure
  // ------------------------------
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // for mac
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);

  // glfw window creation
  // --------------------

  // fullscreen on startup
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
  // ---------------------------------------
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

  // build and compile our shader program
  // ------------------------------------
  Shader ourShader("Shader/texture.vs", "Shader/texture.fs");

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

  // load and create a texture
  // -------------------------
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D,
                texture);  // all upcoming GL_TEXTURE_2D operations now have
                           // effect on this texture object
  // set the texture wrapping parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                  GL_REPEAT);  // set texture wrapping to GL_REPEAT (default
                               // wrapping method)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // set texture filtering parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // load image, create texture and generate mipmaps
  int width, height, nrChannels;
  unsigned char* data =
      stbi_load("assets/container.jpg", &width, &height, &nrChannels, 0);
  if (data) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    std::cout << "Failed to load texture" << std::endl;
  }
  stbi_image_free(data);

  // render loop
  // -----------
  while (!glfwWindowShouldClose(window)) {
    // calculate delta time
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // input
    // -----
    glfwSetCursorPosCallback(window, mouse_callback);
    processInput(window);

    // render
    // ------

    // imgui
    // 1. Start ImGui Frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Performance");
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::End();

    // opengl
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // bind Texture
    glBindTexture(GL_TEXTURE_2D, texture);

    // matrices
    // -----------
    // global space
    glm::mat4 model = glm::mat4(1.0f);

    // apply gravity & floor collision

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

    // view matrix
    glm::mat4 view = camera.GetViewMatrix();

    // projection matrix
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    // Ensure we don't divide by zero if window is minimized
    float aspect = (height > 0) ? (float)width / (float)height : 1.0f;

    // Use the dynamic aspect ratio
    glm::mat4 projection =
        glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

    ourShader.use();
    int modelLoc = glGetUniformLocation(ourShader.ID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    int viewLoc = glGetUniformLocation(ourShader.ID, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    int projectionLoc = glGetUniformLocation(ourShader.ID, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // render container
    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 36, modelMatrices.size());

    // render imgui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved
    // etc.)
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // optional: de-allocate all resources once they've outlived their purpose:
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

// process all input: query GLFW whether relevant keys are pressed/released this
// frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
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
  camera.ProcessKeyboard(window, deltaTime);
  static bool pWasDown = false;

  if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
    if (!pWasDown) {
      wireframe = !wireframe;
      glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    }
    pWasDown = true;
  } else {
    pWasDown = false;
  }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
  (void)window;
  camera.ProcessMouse(xpos, ypos);
}

// glfw: whenever the window size changed (by OS or user resize) this callback
// function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
  (void)window;
  glViewport(0, 0, width, height);
}
