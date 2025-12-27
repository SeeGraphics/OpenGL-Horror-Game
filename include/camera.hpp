#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>

struct GLFWwindow;

class Camera {
 public:
  glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
  glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
  glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

  glm::vec3 direction;
  float yaw = -90.0;
  float pitch = 0.0;

  bool mouseDisabled = true;

  void AttachToWindow(GLFWwindow* window, float screenX, float screenY);
  void ProcessKeyboard(GLFWwindow* window, float deltaTime);
  void ProcessMouse(double xpos, double ypos);
  glm::mat4 GetViewMatrix();

 private:
  bool firstMouse = true;
  float lastX = 0.0, lastY = 0.0;
};

#endif
