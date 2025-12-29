#include "scene/world.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "app.hpp"

void buildFloor(AppState& state) {
  state.modelMatrices.clear();
  state.modelMatrices.reserve(state.floorSize * state.floorSize * 4);
  for (int x = -state.floorSize; x < state.floorSize; x++) {
    for (int z = -state.floorSize; z < state.floorSize; z++) {
      glm::mat4 model = glm::mat4(1.0f);
      float xPos = (float)x * state.cubeScale;
      float zPos = (float)z * state.cubeScale;
      model = glm::translate(model,
                             glm::vec3(xPos, state.floorY, zPos));
      model = glm::scale(model, glm::vec3(state.cubeScale));
      state.modelMatrices.push_back(model);
    }
  }
}

void updateGroundCollision(AppState& state) {
  if (state.freeCam) return;

  if (!state.camera.isGrounded) {
    state.camera.velocity.y += state.camera.GRAVITY * state.deltaTime;
  }

  state.camera.cameraPos += state.camera.velocity * state.deltaTime;
  float floorLevel = state.floorY + state.camera.cameraHeight;

  if (state.camera.cameraPos.y <= floorLevel) {
    state.camera.cameraPos.y = floorLevel;  // Snap to floor
    state.camera.velocity.y = 0.0f;         // Stop falling
    state.camera.isGrounded = true;
  } else {
    state.camera.isGrounded = false;
  }
}
