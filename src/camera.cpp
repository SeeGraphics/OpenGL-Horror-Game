#include "camera.hpp"

#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// constructor for the vars
Camera::Camera() {
  cameraHeight = 1.5f;  // eye level changable (e.g crouching)
  cameraPos = glm::vec3(0.0f, cameraHeight, 3.0f);
  cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
  cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

  yaw = -90.0;
  pitch = 0.0;

  // gravity and physics vars
  GRAVITY = -9.81f;
  jumpforce = 5.0f;
  // float playerHeight = 2.0f;  // for collision
  velocity = glm::vec3(0.0f);
  isGrounded = false;

  // for ungrabbing mouse with ´q´
  mouseDisabled = true;
}

void Camera::AttachToWindow(GLFWwindow* window, float screenX, float screenY) {
  lastX = screenX / 2.0f;
  lastY = screenY / 2.0f;
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Camera::ProcessKeyboard(GLFWwindow* window, float deltaTime,
                             bool freeCam) {
  float cameraSpeed = 4.0f * deltaTime;
  float cameraSpeedSneak = 2.0f * deltaTime;
  float cameraSpeedSprint = 6.0f * deltaTime;
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    if (mouseDisabled) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      mouseDisabled = false;
    } else {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      mouseDisabled = true;
    }
  }

  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    if (freeCam) {
      cameraPos += cameraSpeed * cameraUp;
    } else {
      if (isGrounded) {
        velocity.y = jumpforce;
        isGrounded = false;
      }
    }
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
  }
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    if (freeCam) {
      cameraPos -= cameraSpeed * cameraUp;
    }
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    cameraPos += cameraSpeed * cameraFront;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    cameraPos -= cameraSpeed * cameraFront;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    cameraPos -=
        glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    cameraPos +=
        glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

void Camera::ProcessMouse(double xpos, double ypos) {
  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }
  float xoffset = xpos - lastX;
  float yoffset =
      lastY - ypos;  // reversed since y-coordinates go from bottom to top
  lastX = xpos;
  lastY = ypos;
  const float sensitivity = 0.1f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;
  yaw += xoffset;
  pitch += yoffset;
  if (pitch > 89.0f) pitch = 89.0f;
  if (pitch < -89.0f) pitch = -89.0f;
  direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  direction.y = sin(glm::radians(pitch));
  direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  cameraFront = glm::normalize(direction);
}

glm::mat4 Camera::GetViewMatrix() {
  return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}
