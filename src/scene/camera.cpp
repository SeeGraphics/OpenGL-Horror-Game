#include "scene/camera.hpp"

#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void Cooldown::Update(float deltaTime) {
  if (timer > 0.0f) {
    timer -= deltaTime;
    if (timer < 0.0f) {
      timer = 0.0f;
    }
  }
}

bool Cooldown::TryUse() {
  if (timer > 0.0f) {
    return false;
  }
  timer = duration;
  return true;
}

// constructor for the vars
Camera::Camera() {
  cameraHeight = 1.5f;  // eye level, changable (e.g crouching)

  cameraPos = glm::vec3(5.95f, 8.45f, -8.16f);
  cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
  flatFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
  cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
  wishDir = glm::vec3(0.0f);

  yaw = -90.0;
  pitch = 0.0;

  // gravity and physics vars
  GRAVITY = -9.81f;
  jumpforce = 3.0f;
  velocity = glm::vec3(0.0f);
  isGrounded = false;
  jumpCooldown.duration = 0.8f;

  // for ungrabbing mouse with ´q´
  mouseDisabled = true;
  flashlightEnabled = false;
  flashlightToggled = false;
  flashlightToggleCooldown.duration = 0.2f;
  isMoving = false;
  isSprinting = false;

  // bobbing settings
  bobbingAmount = 0.03f;
  bobbingSpeed = 3.0f;
  bobTimer = 0.0f;
  visualBobOffset = 0.0f;
}

void Camera::AttachToWindow(GLFWwindow* window, float screenX, float screenY) {
  lastX = screenX / 2.0f;
  lastY = screenY / 2.0f;
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Camera::ProcessKeyboard(GLFWwindow* window, float deltaTime,
                             bool freeCam) {
  // update cooldowns
  flashlightToggleCooldown.Update(deltaTime);
  jumpCooldown.Update(deltaTime);

  // movement
  glm::vec3 wishDir = glm::vec3(0.0f);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) wishDir += flatFront;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) wishDir -= flatFront;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    wishDir -= glm::normalize(glm::cross(flatFront, cameraUp));
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    wishDir += glm::normalize(glm::cross(flatFront, cameraUp));

  // states and vars
  static bool cWasDown = false;
  static bool isSneaking = false;
  float baseSpeed = 3.0f;
  bool hasInput = glm::length(wishDir) > 0.0f;
  isMoving = hasInput;
  isSprinting = false;

  // Calculate base speed
  float currentSpeed = baseSpeed;

  // Sprinting Logic with "Human" constraints
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && !freeCam &&
      hasInput) {
    // Only allow sprinting if moving generally FORWARD
    // Dot product tells us if wishDir and flatFront point in the same direction
    glm::vec3 wishDirNorm = glm::normalize(wishDir);
    float movementDirectionMatch = glm::dot(wishDirNorm, flatFront);

    if (movementDirectionMatch > 0.5f) {  // Moving mostly forward
      currentSpeed = 5.0f;
      isSprinting = true;
    } else {
      // Trying to sprint backwards or sideways? No boost for you.
      currentSpeed = baseSpeed;
    }
  }

  if (isSneaking && !freeCam) {
    currentSpeed = 1.5f;
    isSprinting = false;
  }

  float velocityValue = currentSpeed * deltaTime;

  // toggle mouse (ungrab)
  static bool qWasDown = false;
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    if (!qWasDown) {
      if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true;  // reset firstMouse so the camera doesn't "jump"
      }
    }
    qWasDown = true;
  } else {
    qWasDown = false;
  }

  // toggle flashlight
  static bool fWasDown = false;
  if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
    if (!fWasDown) {
      if (flashlightToggleCooldown.TryUse()) {
        flashlightEnabled = !flashlightEnabled;
        flashlightToggled = true;
      }
    }
    fWasDown = true;
  } else {
    fWasDown = false;
  }

  // Jump
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    if (freeCam) {
      cameraPos += velocityValue * cameraUp;
    } else {
      if (isGrounded) {
        if (jumpCooldown.TryUse()) {
          velocity.y = jumpforce;
          isGrounded = false;
        }
      }
    }
  }

  // Sneak
  // Sneak Toggle
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
    if (freeCam) {
      cameraPos -= velocityValue * cameraUp;
    } else {
      if (!cWasDown) {
        isSneaking = !isSneaking;  // Just flip the state
      }
      cWasDown = true;
    }
  } else {
    cWasDown = false;
  }

  // Smooth Sneak Transition
  float targetHeight = isSneaking ? 1.0f : 1.5f;
  float sneakSpeed = 8.0f;  // Adjust for faster/slower crouch
  cameraHeight = glm::mix(cameraHeight, targetHeight, sneakSpeed * deltaTime);

  // Apply Movement Logic
  if (freeCam) {
    // Fly mode: Instant response
    if (glm::length(wishDir) > 0.0f) {
      currentSpeed = 20.0f;
      cameraPos += glm::normalize(wishDir) * currentSpeed * deltaTime;
    }
    velocity = glm::vec3(0.0f);  // Kill momentum when switching to freeCam
  } else {
    if (isGrounded) {
      if (glm::length(wishDir) > 0.0f) {
        wishDir = glm::normalize(wishDir);

        // ACCELERATION: Nudge current velocity toward wishDir
        float accel = isSneaking ? 25.0f : 50.0f;  // Slower accel when sneaking
        velocity.x += wishDir.x * accel * deltaTime;
        velocity.z += wishDir.z * accel * deltaTime;

        // CAP SPEED: Don't exceed currentSpeed (baseSpeed or sprintSpeed)
        float mag = glm::length(glm::vec2(velocity.x, velocity.z));
        if (mag > currentSpeed) {
          float ratio = currentSpeed / mag;
          velocity.x *= ratio;
          velocity.z *= ratio;
        }
      } else {
        // DECELERATION (Friction): Slow down when no keys are pressed
        float friction = 15.0f;
        float drop = friction * deltaTime;
        float mag = glm::length(glm::vec2(velocity.x, velocity.z));

        if (mag > 0.0f) {
          float newSpeed = mag - drop;
          if (newSpeed < 0.0f) newSpeed = 0.0f;
          float ratio = newSpeed / mag;
          velocity.x *= ratio;
          velocity.z *= ratio;
        }
      }
    }
    // Note: If !isGrounded, we don't touch velocity.x/z.
    // The physics loop in main.cpp will keep moving cameraPos by this velocity.
  }

  // Head Bob Logic
  float horizontalSpeed = glm::length(glm::vec2(velocity.x, velocity.z));

  // Only increment the timer if we are grounded
  if (isGrounded && horizontalSpeed > 0.1f) {
    bobTimer += horizontalSpeed * deltaTime * bobbingSpeed;
    float targetBob = sin(bobTimer) * bobbingAmount;
    visualBobOffset = glm::mix(visualBobOffset, targetBob, 15.0f * deltaTime);
  } else {
    visualBobOffset = glm::mix(visualBobOffset, 0.0f, 10.0f * deltaTime);
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
  // get direction based on where we look and normalize
  direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  direction.y = sin(glm::radians(pitch));
  direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  cameraFront = glm::normalize(direction);  // for freeCam only
  flatFront = glm::normalize(
      glm::vec3(cameraFront.x, 0.0f,
                cameraFront.z));  // so looking down doesnt stop all momentum
}

glm::mat4 Camera::GetViewMatrix() {
  glm::vec3 bobbedPos = cameraPos;
  bobbedPos.y += visualBobOffset;

  return glm::lookAt(bobbedPos, bobbedPos + cameraFront, cameraUp);
}
