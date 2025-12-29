#include "scene/world.hpp"

#include <cmath>
#include <iostream>
#include <vector>

#include <stb_image.h>

#include "app.hpp"

static bool heightmapLoaded = false;
static bool heightmapFailed = false;
static bool heightmapLogged = false;
static int heightmapWidth = 0;
static int heightmapHeight = 0;
static std::vector<unsigned short> heightmapData;

static float clampFloat(float value, float minValue, float maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

static float hashToUnitFloat(int x, int z, int seed) {
  unsigned int h = static_cast<unsigned int>(x);
  h = h * 374761393u + static_cast<unsigned int>(z) * 668265263u;
  h ^= static_cast<unsigned int>(seed) * 1274126177u;
  h ^= h >> 13;
  h *= 1274126177u;
  h ^= h >> 16;
  return (h & 0x00FFFFFF) / 16777216.0f;
}

static bool loadHeightmap() {
  if (heightmapLoaded || heightmapFailed) {
    return heightmapLoaded;
  }

  int channels = 0;
  unsigned short* data = stbi_load_16("assets/heightmap_16bit.png",
                                      &heightmapWidth, &heightmapHeight,
                                      &channels, 1);
  if (!data) {
    std::cout << "Heightmap failed to load" << std::endl;
    heightmapFailed = true;
    return false;
  }

  heightmapData.assign(data, data + (heightmapWidth * heightmapHeight));
  stbi_image_free(data);
  heightmapLoaded = true;

  if (!heightmapLogged) {
    unsigned short minValue = 65535;
    unsigned short maxValue = 0;
    for (unsigned short value : heightmapData) {
      if (value < minValue) minValue = value;
      if (value > maxValue) maxValue = value;
    }
    std::cout << "Heightmap loaded: " << heightmapWidth << "x"
              << heightmapHeight << " min=" << minValue << " max=" << maxValue
              << std::endl;
    heightmapLogged = true;
  }

  return true;
}

static float sampleHeightNormalized(float u, float v) {
  if (!loadHeightmap()) {
    return 0.0f;
  }

  u = clampFloat(u, 0.0f, 1.0f);
  v = clampFloat(v, 0.0f, 1.0f);

  float x = u * static_cast<float>(heightmapWidth - 1);
  float y = v * static_cast<float>(heightmapHeight - 1);
  int x0 = static_cast<int>(std::floor(x));
  int y0 = static_cast<int>(std::floor(y));
  int x1 = x0 + 1;
  int y1 = y0 + 1;
  if (x1 >= heightmapWidth) x1 = heightmapWidth - 1;
  if (y1 >= heightmapHeight) y1 = heightmapHeight - 1;
  float tx = x - static_cast<float>(x0);
  float ty = y - static_cast<float>(y0);

  unsigned short h00 = heightmapData[y0 * heightmapWidth + x0];
  unsigned short h10 = heightmapData[y0 * heightmapWidth + x1];
  unsigned short h01 = heightmapData[y1 * heightmapWidth + x0];
  unsigned short h11 = heightmapData[y1 * heightmapWidth + x1];

  float h00f = static_cast<float>(h00) / 65535.0f;
  float h10f = static_cast<float>(h10) / 65535.0f;
  float h01f = static_cast<float>(h01) / 65535.0f;
  float h11f = static_cast<float>(h11) / 65535.0f;

  float h0 = h00f + (h10f - h00f) * tx;
  float h1 = h01f + (h11f - h01f) * tx;
  return h0 + (h1 - h0) * ty;
}

static float getTerrainHeightAt(const AppState& state, float worldX,
                                float worldZ) {
  int gridSize = state.floorSize * 2;
  float tileSize = state.cubeScale;
  float start = (-static_cast<float>(state.floorSize) - 0.5f) * tileSize;
  float span = static_cast<float>(gridSize) * tileSize;
  if (span <= 0.0f) {
    return state.floorY + (state.cubeScale * 0.5f);
  }

  float u = (worldX - start) / span;
  float v = (worldZ - start) / span;
  u = clampFloat(u, 0.0f, 1.0f);
  v = clampFloat(v, 0.0f, 1.0f);

  float baseHeight = state.floorY + (state.cubeScale * 0.5f);
  float heightValue = sampleHeightNormalized(u, v);
  return baseHeight + (heightValue * state.heightmapScale);
}

void buildFloor(AppState& state) {
  int gridSize = state.floorSize * 2;
  float tileSize = state.cubeScale;
  float start = (-static_cast<float>(state.floorSize) - 0.5f) * tileSize;
  int vertCount = gridSize + 1;

  state.terrainVertices.clear();
  state.terrainIndices.clear();
  state.terrainVertices.reserve(vertCount * vertCount * 8);
  state.terrainIndices.reserve(gridSize * gridSize * 6);

  std::vector<float> heights;
  heights.resize(vertCount * vertCount, 0.0f);
  float baseHeight = state.floorY + (state.cubeScale * 0.5f);
  for (int z = 0; z <= gridSize; ++z) {
    for (int x = 0; x <= gridSize; ++x) {
      float u = static_cast<float>(x) / static_cast<float>(gridSize);
      float v = static_cast<float>(z) / static_cast<float>(gridSize);
      float heightValue = sampleHeightNormalized(u, v);
      heights[z * vertCount + x] =
          baseHeight + (heightValue * state.heightmapScale);
    }
  }

  for (int z = 0; z <= gridSize; ++z) {
    float zPos = start + z * tileSize;
    for (int x = 0; x <= gridSize; ++x) {
      float xPos = start + x * tileSize;
      float u = static_cast<float>(x);
      float v = static_cast<float>(z);
      float height = heights[z * vertCount + x];

      int xPrev = (x > 0) ? (x - 1) : x;
      int xNext = (x < gridSize) ? (x + 1) : x;
      int zPrev = (z > 0) ? (z - 1) : z;
      int zNext = (z < gridSize) ? (z + 1) : z;

      float heightL = heights[z * vertCount + xPrev];
      float heightR = heights[z * vertCount + xNext];
      float heightD = heights[zPrev * vertCount + x];
      float heightU = heights[zNext * vertCount + x];

      float normalX = heightL - heightR;
      float normalY = 2.0f * tileSize;
      float normalZ = heightD - heightU;
      float normalLen =
          std::sqrt(normalX * normalX + normalY * normalY +
                    normalZ * normalZ);
      if (normalLen > 0.0f) {
        normalX /= normalLen;
        normalY /= normalLen;
        normalZ /= normalLen;
      } else {
        normalX = 0.0f;
        normalY = 1.0f;
        normalZ = 0.0f;
      }

      state.terrainVertices.push_back(xPos);
      state.terrainVertices.push_back(height);
      state.terrainVertices.push_back(zPos);

      state.terrainVertices.push_back(normalX);
      state.terrainVertices.push_back(normalY);
      state.terrainVertices.push_back(normalZ);

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
      state.terrainIndices.push_back(topRight);
      state.terrainIndices.push_back(bottomLeft);

      state.terrainIndices.push_back(topRight);
      state.terrainIndices.push_back(bottomRight);
      state.terrainIndices.push_back(bottomLeft);
    }
  }
}

void buildGrass(AppState& state) {
  int gridSize = state.floorSize * 2;
  float tileSize = state.cubeScale;
  float start = (-static_cast<float>(state.floorSize) - 0.5f) * tileSize;
  float density = state.grassDensity;
  if (density < 0.0f) density = 0.0f;

  state.grassInstances.clear();

  if (density <= 0.0f) {
    return;
  }

  float nearRadius = state.grassNearRadius;
  float farRadius = state.grassFarRadius;
  if (nearRadius < 0.0f) nearRadius = 0.0f;
  if (farRadius < nearRadius) farRadius = nearRadius;

  if (farRadius <= 0.0f) {
    return;
  }

  float centerX = state.camera.cameraPos.x;
  float centerZ = state.camera.cameraPos.z;
  state.grassCenter = glm::vec2(centerX, centerZ);

  float minXf = ((centerX - farRadius) - start) / tileSize - 0.5f;
  float maxXf = ((centerX + farRadius) - start) / tileSize - 0.5f;
  float minZf = ((centerZ - farRadius) - start) / tileSize - 0.5f;
  float maxZf = ((centerZ + farRadius) - start) / tileSize - 0.5f;
  int minX = static_cast<int>(std::floor(minXf));
  int maxX = static_cast<int>(std::floor(maxXf));
  int minZ = static_cast<int>(std::floor(minZf));
  int maxZ = static_cast<int>(std::floor(maxZf));

  if (minX < 0) minX = 0;
  if (minZ < 0) minZ = 0;
  if (maxX > gridSize - 1) maxX = gridSize - 1;
  if (maxZ > gridSize - 1) maxZ = gridSize - 1;

  if (minX > maxX || minZ > maxZ) {
    return;
  }

  int maxInstancesPerTile = static_cast<int>(std::ceil(density));
  size_t tileCount =
      static_cast<size_t>(maxX - minX + 1) * static_cast<size_t>(maxZ - minZ + 1);
  state.grassInstances.reserve(tileCount *
                               static_cast<size_t>(maxInstancesPerTile));

  float nearRadiusSq = nearRadius * nearRadius;
  float farRadiusSq = farRadius * farRadius;
  float falloffSpan = farRadius - nearRadius;

  for (int z = minZ; z <= maxZ; ++z) {
    float tileCenterZ = start + (static_cast<float>(z) + 0.5f) * tileSize;
    for (int x = minX; x <= maxX; ++x) {
      float tileCenterX = start + (static_cast<float>(x) + 0.5f) * tileSize;
      float dx = tileCenterX - centerX;
      float dz = tileCenterZ - centerZ;
      float distSq = dx * dx + dz * dz;

      if (distSq > farRadiusSq) {
        continue;
      }

      float densityScale = 1.0f;
      if (distSq > nearRadiusSq && falloffSpan > 0.0f) {
        float distance = std::sqrt(distSq);
        float t = (distance - nearRadius) / falloffSpan;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        densityScale = 1.0f + (state.grassMidDensity - 1.0f) * t;
      }

      float tileDensity = density * densityScale;
      if (tileDensity <= 0.0f) {
        continue;
      }

      int baseCount = static_cast<int>(std::floor(tileDensity));
      float extraChance = tileDensity - static_cast<float>(baseCount);
      int instanceCount = baseCount;
      if (extraChance > 0.0f) {
        float roll = hashToUnitFloat(x, z, 3);
        if (roll < extraChance) {
          instanceCount += 1;
        }
      }

      if (instanceCount == 0) {
        continue;
      }

      for (int i = 0; i < instanceCount; ++i) {
        int seedBase = 10 + (i * 2);
        float offsetX = (hashToUnitFloat(x, z, seedBase) - 0.5f) * tileSize;
        float offsetZ = (hashToUnitFloat(x, z, seedBase + 1) - 0.5f) * tileSize;
        float xPos = tileCenterX + offsetX;
        float zPos = tileCenterZ + offsetZ;
        float yPos = getTerrainHeightAt(state, xPos, zPos);
        state.grassInstances.push_back(glm::vec3(xPos, yPos + 0.01f, zPos));
      }
    }
  }
}

void updateGroundCollision(AppState& state) {
  if (state.freeCam) return;

  bool wasGrounded = state.camera.isGrounded;

  if (!state.camera.isGrounded) {
    state.camera.velocity.y += state.camera.GRAVITY * state.deltaTime;
  }

  state.camera.cameraPos += state.camera.velocity * state.deltaTime;

  float groundHeight = getTerrainHeightAt(
      state, state.camera.cameraPos.x, state.camera.cameraPos.z);
  float floorLevel = groundHeight + state.camera.cameraHeight;

  float groundStickEpsilon = 0.2f * state.cubeScale;
  float distance = state.camera.cameraPos.y - floorLevel;

  if (state.camera.velocity.y <= 0.0f) {
    if (wasGrounded) {
      if (distance <= groundStickEpsilon) {
        state.camera.cameraPos.y = floorLevel;
        state.camera.velocity.y = 0.0f;
        state.camera.isGrounded = true;
        return;
      }
    } else {
      if (distance <= 0.0f) {
        state.camera.cameraPos.y = floorLevel;
        state.camera.velocity.y = 0.0f;
        state.camera.isGrounded = true;
        return;
      }
    }
  }

  state.camera.isGrounded = false;
}
