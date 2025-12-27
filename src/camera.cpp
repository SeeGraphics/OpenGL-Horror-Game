#include "camera.hpp"

#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// constructor for the vars
Camera::Camera() {
  cameraHeight = 1.5f;  // eye level changable (e.g crouching)
  playerHeight = 1.5f;  // fixed, cause its used for floor Level

  cameraPos = glm::vec3(0.0f, cameraHeight, 3.0f);
  cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
  flatFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
  cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

  yaw = 0.0;
  pitch = 0.0;

  // gravity and physics vars
  GRAVITY = -9.81f;
  jumpforce = 3.0f;
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
  // states and vars
  static bool cWasDown = false;
  static bool isSneaking = false;
  float baseSpeed = 4.0f;

  // check current speed (for crouching and later maybe stamina)
  float currentSpeed = baseSpeed;

  // speed up when sprinting
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    if (freeCam)
      currentSpeed = 30.0f;
    else
      currentSpeed = 6.5f;
  }
  // slow down when sneaking
  if (isSneaking && !freeCam) {
    currentSpeed = 1.5f;
  }

  float velocityValue = currentSpeed * deltaTime;

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
      cameraPos += velocityValue * cameraUp;
    } else {
      if (isGrounded) {
        velocity.y = jumpforce;
        isGrounded = false;
      }
    }
  }

  // sneak / move down (if freecam)
  // only handles hight and boolean
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
    if (freeCam) {
      cameraPos -= velocityValue * cameraUp;  // Move down in fly mode
    } else {
      if (!cWasDown) {
        isSneaking = !isSneaking;

        if (isSneaking) {
          cameraHeight = 1.0f;
          cameraPos.y -= (1.5f - 1.0f);
        } else {
          cameraHeight = 1.5f;
          cameraPos.y += (1.5f - 1.0f);
        }
      }
      cWasDown = true;  // Mark as held
    }
  } else {
    cWasDown = false;  // Reset when key is released
  }
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    if (freeCam)
      cameraPos += velocityValue * cameraFront;
    else
      cameraPos += velocityValue * flatFront;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    if (freeCam)
      cameraPos -= velocityValue * cameraFront;
    else
      cameraPos -= velocityValue * flatFront;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    if (freeCam)
      cameraPos -=
          glm::normalize(glm::cross(cameraFront, cameraUp)) * velocityValue;
    else
      cameraPos -=
          glm::normalize(glm::cross(flatFront, cameraUp)) * velocityValue;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    if (freeCam)
      cameraPos +=
          glm::normalize(glm::cross(cameraFront, cameraUp)) * velocityValue;
    else
      cameraPos +=
          glm::normalize(glm::cross(flatFront, cameraUp)) * velocityValue;
  }
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
  flatFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
}

glm::mat4 Camera::GetViewMatrix() {
  return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}
