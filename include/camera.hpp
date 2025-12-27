#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>

struct GLFWwindow;

class Camera {
 public:
  Camera();
  float cameraHeight;  // eye level changable (e.g crouching)
  float playerHeight;  // floor level, dont change
  glm::vec3 cameraPos;
  glm::vec3 cameraFront;  // for flying
  glm::vec3 flatFront;    // for walking, avoids not moving when looking down
  glm::vec3 cameraUp;

  glm::vec3 direction;
  float yaw;
  float pitch;

  // gravity and physics vars
  float GRAVITY;
  float jumpforce;
  glm::vec3 velocity;
  bool isGrounded;

  // for ungrabbing mouse with ´q´
  bool mouseDisabled;

  void AttachToWindow(GLFWwindow* window, float screenX, float screenY);
  void ProcessKeyboard(GLFWwindow* window, float deltaTime, bool freeCam);
  void ProcessMouse(double xpos, double ypos);
  glm::mat4 GetViewMatrix();

 private:
  bool firstMouse = true;
  float lastX = 0.0, lastY = 0.0;
};

#endif
