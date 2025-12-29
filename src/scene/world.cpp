#include "scene/world.hpp"

#include "app.hpp"

void buildFloor(AppState& state) {
  int gridSize = state.floorSize * 2;
  float tileSize = state.cubeScale;
  float start = (-static_cast<float>(state.floorSize) - 0.5f) * tileSize;
  float height = state.floorY + (state.cubeScale * 0.5f);
  int vertCount = gridSize + 1;

  state.terrainVertices.clear();
  state.terrainIndices.clear();
  state.terrainVertices.reserve(vertCount * vertCount * 8);
  state.terrainIndices.reserve(gridSize * gridSize * 6);

  for (int z = 0; z <= gridSize; ++z) {
    float zPos = start + z * tileSize;
    for (int x = 0; x <= gridSize; ++x) {
      float xPos = start + x * tileSize;
      float u = static_cast<float>(x);
      float v = static_cast<float>(z);

      state.terrainVertices.push_back(xPos);
      state.terrainVertices.push_back(height);
      state.terrainVertices.push_back(zPos);

      state.terrainVertices.push_back(0.0f);
      state.terrainVertices.push_back(1.0f);
      state.terrainVertices.push_back(0.0f);

      state.terrainVertices.push_back(u);
      state.terrainVertices.push_back(v);
    }
  }

  int stride = vertCount;
  for (int z = 0; z < gridSize; ++z) {
    for (int x = 0; x < gridSize; ++x) {
      unsigned int topLeft = z * stride + x;
      unsigned int topRight = topLeft + 1;
      unsigned int bottomLeft = (z + 1) * stride + x;
      unsigned int bottomRight = bottomLeft + 1;

      state.terrainIndices.push_back(topLeft);
      state.terrainIndices.push_back(bottomLeft);
      state.terrainIndices.push_back(topRight);

      state.terrainIndices.push_back(topRight);
      state.terrainIndices.push_back(bottomLeft);
      state.terrainIndices.push_back(bottomRight);
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
