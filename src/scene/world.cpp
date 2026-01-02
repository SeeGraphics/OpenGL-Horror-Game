#include "scene/world.hpp"

#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

#include "app.hpp"

static bool heightmapLoaded = false;
static bool heightmapFailed = false;
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

static int addModelAsset(AppState& state, const char* id, const char* path,
                         const ModelRenderSettings& settings) {
  state.modelAssets.emplace_back();
  ModelAsset& asset = state.modelAssets.back();
  if (id) {
    asset.id = id;
  }
  if (path) {
    asset.path = path;
  }
  asset.renderSettings = settings;
  if (!asset.path.empty()) {
    asset.model.Load(asset.path.c_str(), asset.renderSettings);
  }
  return static_cast<int>(state.modelAssets.size()) - 1;
}

int addModelInstance(AppState& state, int assetIndex, const glm::vec3& position,
                     const glm::vec3& rotation, const glm::vec3& scale,
                     bool isEditorPlaced) {
  if (assetIndex < 0 ||
      assetIndex >= static_cast<int>(state.modelAssets.size())) {
    return -1;
  }

  ModelInstance instance;
  instance.assetIndex = assetIndex;
  instance.position = position;
  instance.rotation = rotation;
  instance.scale = scale;
  instance.isEditorPlaced = isEditorPlaced;
  state.modelInstances.push_back(instance);
  return static_cast<int>(state.modelInstances.size()) - 1;
}

static int findInstanceIndexByAsset(const AppState& state, int assetIndex,
                                    bool requireNonEditor) {
  if (assetIndex < 0) {
    return -1;
  }
  for (int i = static_cast<int>(state.modelInstances.size()) - 1; i >= 0; --i) {
    const ModelInstance& instance = state.modelInstances[i];
    if (instance.assetIndex == assetIndex &&
        (!requireNonEditor || !instance.isEditorPlaced)) {
      return i;
    }
  }
  return -1;
}

static void scatterTrees(AppState& state, int assetIndex) {
  if (assetIndex < 0) {
    return;
  }

  const float cellSize = state.cubeScale * 8.0f;
  float density = state.treeDensity;
  if (density < 0.0f) density = 0.0f;
  if (density > 8.0f) density = 8.0f;
  const float baseScale = 0.003f;
  const float scaleJitter = 0.35f;  // different tree sizes
  const float jitterScale = 1.0f;  // keep offsets inside each cell

  int gridSize = state.floorSize * 2;
  float tileSize = state.cubeScale;
  float start = (-static_cast<float>(state.floorSize) - 0.5f) * tileSize;
  float span = static_cast<float>(gridSize) * tileSize;
  if (cellSize <= 0.0f || span <= 0.0f) {
    return;
  }

  int cellsX = static_cast<int>(std::ceil(span / cellSize));
  int cellsZ = static_cast<int>(std::ceil(span / cellSize));
  float maxPos = start + span;

  int instanceStart = static_cast<int>(state.modelInstances.size());
  int spawned = 0;
  // Minimum spacing between trees (XZ plane).
  float minDistance = state.cubeScale * 2.0f;
  float minDistanceSq = minDistance * minDistance;

  // Spatial grid for fast neighbor checks (bucketed by minDistance).
  int spacingCellsX = static_cast<int>(std::ceil(span / minDistance));
  int spacingCellsZ = static_cast<int>(std::ceil(span / minDistance));
  if (spacingCellsX < 1) spacingCellsX = 1;
  if (spacingCellsZ < 1) spacingCellsZ = 1;
  std::vector<std::vector<int>> spacingGrid(
      static_cast<size_t>(spacingCellsX) *
      static_cast<size_t>(spacingCellsZ));
  std::vector<glm::vec3> placedPositions;
  placedPositions.reserve(static_cast<size_t>(cellsX * cellsZ));

  // Map a world position to a spacing grid cell.
  auto getSpacingCell = [&](float xPos, float zPos, int& gx, int& gz) {
    gx = static_cast<int>(std::floor((xPos - start) / minDistance));
    gz = static_cast<int>(std::floor((zPos - start) / minDistance));
    if (gx < 0 || gz < 0 || gx >= spacingCellsX || gz >= spacingCellsZ) {
      return false;
    }
    return true;
  };

  // Check the 3x3 neighbor buckets for a close tree.
  auto canPlace = [&](float xPos, float zPos) {
    int gx = 0;
    int gz = 0;
    if (!getSpacingCell(xPos, zPos, gx, gz)) {
      return false;
    }
    int minX = std::max(0, gx - 1);
    int maxX = std::min(spacingCellsX - 1, gx + 1);
    int minZ = std::max(0, gz - 1);
    int maxZ = std::min(spacingCellsZ - 1, gz + 1);
    for (int nz = minZ; nz <= maxZ; ++nz) {
      for (int nx = minX; nx <= maxX; ++nx) {
        const std::vector<int>& bucket =
            spacingGrid[static_cast<size_t>(nz) *
                            static_cast<size_t>(spacingCellsX) +
                        static_cast<size_t>(nx)];
        for (int index : bucket) {
          const glm::vec3& other = placedPositions[index];
          float dx = other.x - xPos;
          float dz = other.z - zPos;
          if ((dx * dx + dz * dz) < minDistanceSq) {
            return false;
          }
        }
      }
    }
    return true;
  };

  // Iterate over coarse cells and place instances with retries.
  for (int z = 0; z < cellsZ; ++z) {
    float cellCenterZ = start + (static_cast<float>(z) + 0.5f) * cellSize;
    if (cellCenterZ < start || cellCenterZ > maxPos) {
      continue;
    }
    for (int x = 0; x < cellsX; ++x) {
      float cellCenterX = start + (static_cast<float>(x) + 0.5f) * cellSize;
      if (cellCenterX < start || cellCenterX > maxPos) {
        continue;
      }

      int baseCount = static_cast<int>(std::floor(density));
      float extraChance = density - static_cast<float>(baseCount);
      int instanceCount = baseCount;
      if (extraChance > 0.0f) {
        float roll = hashToUnitFloat(x, z, 41);
        if (roll < extraChance) {
          instanceCount += 1;
        }
      }
      if (instanceCount == 0) {
        continue;
      }

      int placed = 0;
      int attempt = 0;
      int maxAttempts = std::max(instanceCount * 6, 4);
      // Try multiple candidates to satisfy spacing constraints.
      while (placed < instanceCount && attempt < maxAttempts) {
        int seedBase = 43 + (attempt * 7);
        float offsetX =
            (hashToUnitFloat(x, z, seedBase) - 0.5f) * cellSize * jitterScale;
        float offsetZ = (hashToUnitFloat(x, z, seedBase + 1) - 0.5f) *
                        cellSize * jitterScale;
        float xPos = cellCenterX + offsetX;
        float zPos = cellCenterZ + offsetZ;
        // Discard candidates outside world bounds.
        if (xPos < start || xPos > maxPos || zPos < start || zPos > maxPos) {
          attempt++;
          continue;
        }
        // Enforce min spacing against nearby placements.
        if (!canPlace(xPos, zPos)) {
          attempt++;
          continue;
        }
        float yPos = getTerrainHeightAt(state, xPos, zPos);
        float rotationY = hashToUnitFloat(x, z, seedBase + 3) * 360.0f;
        float scaleRand = hashToUnitFloat(x, z, seedBase + 5);
        float scale =
            baseScale * (1.0f - scaleJitter + (scaleRand * scaleJitter * 2.0f));

        // Record the instance and cache its position in the spacing grid.
        addModelInstance(state, assetIndex, glm::vec3(xPos, yPos, zPos),
                         glm::vec3(0.0f, rotationY, 0.0f), glm::vec3(scale),
                         false);
        int gx = 0;
        int gz = 0;
        if (getSpacingCell(xPos, zPos, gx, gz)) {
          int positionIndex = static_cast<int>(placedPositions.size());
          placedPositions.push_back(glm::vec3(xPos, yPos, zPos));
          spacingGrid[static_cast<size_t>(gz) *
                          static_cast<size_t>(spacingCellsX) +
                      static_cast<size_t>(gx)]
              .push_back(positionIndex);
        }
        spawned++;
        placed++;
        attempt++;
      }
    }
  }

  if (spawned > 0) {
    state.treeInstanceIndex = instanceStart;
  }

  state.treeInstanceDirty = true;
  (void)spawned;
}

static void resolveTreeCollisions(AppState& state) {
  if (state.treeCollisionPositions.empty()) {
    return;
  }

  float treeRadius = state.cubeScale * 0.19f;
  float playerRadius = state.cubeScale * 0.35f;
  float combinedRadius = treeRadius + playerRadius;
  float combinedRadiusSq = combinedRadius * combinedRadius;

  for (const glm::vec3& treePos : state.treeCollisionPositions) {
    float dx = state.camera.cameraPos.x - treePos.x;
    float dz = state.camera.cameraPos.z - treePos.z;
    float distSq = dx * dx + dz * dz;
    if (distSq >= combinedRadiusSq) {
      continue;
    }

    float dist = std::sqrt(distSq);
    float nx = 1.0f;
    float nz = 0.0f;
    if (dist > 0.0001f) {
      nx = dx / dist;
      nz = dz / dist;
    }

    float push = combinedRadius - dist;
    state.camera.cameraPos.x += nx * push;
    state.camera.cameraPos.z += nz * push;

    float velDot = state.camera.velocity.x * nx + state.camera.velocity.z * nz;
    if (velDot < 0.0f) {
      state.camera.velocity.x -= velDot * nx;
      state.camera.velocity.z -= velDot * nz;
    }
  }
}

static bool loadHeightmap() {
  if (heightmapLoaded || heightmapFailed) {
    return heightmapLoaded;
  }

  int channels = 0;
  unsigned short* data =
      stbi_load_16("assets/heightmap_16bit.png", &heightmapWidth,
                   &heightmapHeight, &channels, 1);
  if (!data) {
    std::cout << "Heightmap failed to load" << std::endl;
    heightmapFailed = true;
    return false;
  }

  heightmapData.assign(data, data + (heightmapWidth * heightmapHeight));
  stbi_image_free(data);
  heightmapLoaded = true;

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

float getTerrainHeightAt(const AppState& state, float worldX, float worldZ) {
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
  float resolutionScale = state.terrainResolutionScale;
  if (resolutionScale < 0.1f) {
    resolutionScale = 0.1f;
  }
  float worldSize =
      static_cast<float>(state.floorSize) * 2.0f * state.cubeScale;
  int gridSize = static_cast<int>(std::round(
      (static_cast<float>(state.floorSize) * 2.0f) / resolutionScale));
  if (gridSize < 1) {
    gridSize = 1;
  }
  float tileSize = worldSize / static_cast<float>(gridSize);
  float start = (-static_cast<float>(state.floorSize) - 0.5f) * state.cubeScale;
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
          std::sqrt(normalX * normalX + normalY * normalY + normalZ * normalZ);
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

  float radius = state.grassRenderRadius;
  if (radius < 0.0f) radius = 0.0f;

  if (radius <= 0.0f) {
    return;
  }

  float centerX = state.camera.cameraPos.x;
  float centerZ = state.camera.cameraPos.z;
  state.grassCenter = glm::vec2(centerX, centerZ);

  float minXf = ((centerX - radius) - start) / tileSize - 0.5f;
  float maxXf = ((centerX + radius) - start) / tileSize - 0.5f;
  float minZf = ((centerZ - radius) - start) / tileSize - 0.5f;
  float maxZf = ((centerZ + radius) - start) / tileSize - 0.5f;
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
  size_t tileCount = static_cast<size_t>(maxX - minX + 1) *
                     static_cast<size_t>(maxZ - minZ + 1);
  state.grassInstances.reserve(tileCount *
                               static_cast<size_t>(maxInstancesPerTile));

  float radiusSq = radius * radius;

  for (int z = minZ; z <= maxZ; ++z) {
    float tileCenterZ = start + (static_cast<float>(z) + 0.5f) * tileSize;
    for (int x = minX; x <= maxX; ++x) {
      float tileCenterX = start + (static_cast<float>(x) + 0.5f) * tileSize;
      float dx = tileCenterX - centerX;
      float dz = tileCenterZ - centerZ;
      float distSq = dx * dx + dz * dz;

      if (distSq > radiusSq) {
        continue;
      }

      float tileDensity = density;
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
  resolveTreeCollisions(state);

  float groundHeight = getTerrainHeightAt(state, state.camera.cameraPos.x,
                                          state.camera.cameraPos.z);
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

void initWorldModels(AppState& state) {
  state.modelAssets.clear();
  state.modelInstances.clear();

  // Check if model is loaded with "...AssetIndex"; -1 = not loaded
  // Check if for instance; -1 = no instance was spawned
  // Both declared in src/app.hpp
  // ONLY needed if it needs to communicate with something else, e.g scale
  // slider in imgui ui or respawn / culling, changing pos stuff like that.
  // IF its just placed in the world for testing then you dont need to add these
  // vars.
  state.treeAssetIndex = -1;
  state.treeInstanceIndex = -1;
  state.walterAssetIndex = -1;
  state.walterInstanceIndex = -1;
  state.flashlightAssetIndex = -1;
  state.flashlightInstanceIndex = -1;
  state.churchAssetIndex = -1;
  state.churchInstanceIndex = -1;
  state.deadtreeAssetIndex = -1;
  state.deadtreeInstanceIndex = -1;

  int templateCount = 0;
  const ModelTemplate* templates = GetModelTemplates(&templateCount);

  // Check if model loads temporarily/ local
  int treeAsset = -1;
  int walterAsset = -1;
  int churchAsset = -1;
  int flashlightAsset = -1;
  int deadtreeAsset = -1;

  for (int i = 0; i < templateCount; ++i) {
    const ModelTemplate& entry = templates[i];
    int assetIndex =
        addModelAsset(state, entry.id, entry.path, entry.renderSettings);

    // get model asset
    if (entry.id && std::strcmp(entry.id, "Tree") == 0) {
      treeAsset = assetIndex;
    } else if (entry.id && std::strcmp(entry.id, "WalterWhite") == 0) {
      walterAsset = assetIndex;
    } else if (entry.id && std::strcmp(entry.id, "Church") == 0) {
      churchAsset = assetIndex;
    } else if (entry.id && std::strcmp(entry.id, "Flashlight") == 0) {
      flashlightAsset = assetIndex;
    } else if (entry.id && std::strcmp(entry.id, "Dead_Tree") == 0) {
      deadtreeAsset = assetIndex;
    }
  }

  // now update those global assetIndexes with the local ones after we loaded
  // them
  state.treeAssetIndex = treeAsset;
  state.walterAssetIndex = walterAsset;
  state.flashlightAssetIndex = flashlightAsset;
  state.churchAssetIndex = churchAsset;
  state.deadtreeAssetIndex = deadtreeAsset;
  scatterTrees(state, treeAsset);

  // PLACE MODELS IN WORLD
  // ----------------------

  // add test walter
  // if (walterAsset >= 0) {
  //   state.walterInstanceIndex =
  //       addModelInstance(state, walterAsset, glm::vec3(8.0f, 38.0f, -14.0f),
  //                        glm::vec3(0.0f), glm::vec3(state.walterScale),
  //                        false);
  // }
  //
  // // add test church
  // if (churchAsset >= 0) {
  //   state.churchInstanceIndex =
  //       addModelInstance(state, churchAsset, glm::vec3(15.0f, 45.0f, -14.0f),
  //                        glm::vec3(0.0f), glm::vec3(state.churchScale),
  //                        false);
  // }

  // add flashlight
  if (flashlightAsset >= 0) {
    state.flashlightInstanceIndex = addModelInstance(
        state, flashlightAsset, state.camera.cameraPos, glm::vec3(0.0f),
        glm::vec3(state.flashlightScale), false);
  }

  // // add dead_tree 0.01f is the scale
  // if (deadtreeAsset >= 0) {
  //   state.deadtreeInstanceIndex =
  //       addModelInstance(state, deadtreeAsset, glm::vec3(20.0f, 50.0f,
  //       -16.0f),
  //                        glm::vec3(0.0f), glm::vec3(state.deadtreeScale),
  //                        false);
  // }
}

void rebuildWorldTrees(AppState& state) {
  if (state.treeAssetIndex < 0) {
    return;
  }

  state.modelInstances.erase(
      std::remove_if(state.modelInstances.begin(), state.modelInstances.end(),
                     [&](const ModelInstance& instance) {
                       return instance.assetIndex == state.treeAssetIndex;
                     }),
      state.modelInstances.end());

  state.treeInstanceIndex = -1;
  scatterTrees(state, state.treeAssetIndex);
}

void updateWorldModelHeights(AppState& state) {
  if (state.treeAssetIndex < 0) {
    return;
  }

  for (ModelInstance& instance : state.modelInstances) {
    if (instance.assetIndex != state.treeAssetIndex) {
      continue;
    }
    float yPos =
        getTerrainHeightAt(state, instance.position.x, instance.position.z);
    instance.position.y = yPos;
  }
}

void updateFlashlightAttachment(AppState& state) {
  if (state.flashlightAssetIndex < 0) {
    return;
  }

  int instanceIndex = state.flashlightInstanceIndex;
  if (instanceIndex < 0 ||
      instanceIndex >= static_cast<int>(state.modelInstances.size()) ||
      state.modelInstances[instanceIndex].assetIndex !=
          state.flashlightAssetIndex ||
      state.modelInstances[instanceIndex].isEditorPlaced) {
    instanceIndex =
        findInstanceIndexByAsset(state, state.flashlightAssetIndex, true);
    state.flashlightInstanceIndex = instanceIndex;
  }

  if (instanceIndex < 0) {
    return;
  }

  ModelInstance& instance = state.modelInstances[instanceIndex];

  glm::vec3 forward = glm::normalize(state.camera.cameraFront);
  glm::vec3 right = glm::cross(forward, state.camera.cameraUp);
  if (glm::length(right) < 0.001f) {
    right = glm::cross(state.camera.flatFront, state.camera.cameraUp);
  }
  right = glm::normalize(right);
  glm::vec3 up = glm::normalize(glm::cross(right, forward));

  float forwardOffset = state.cubeScale * state.flashlightOffsetForward;
  float rightOffset = state.cubeScale * state.flashlightOffsetRight;
  float downOffset = state.cubeScale * state.flashlightOffsetDown;
  float yawOffset = 0.0f;
  float pitchOffset = 0.0f;
  float rollOffset = 0.0f;

  glm::vec3 position = state.camera.cameraPos + (forward * forwardOffset) +
                       (right * rightOffset) + (up * downOffset);
  position.y += state.camera.visualBobOffset;
  instance.position = position;
  instance.rotation = glm::vec3(pitchOffset, yawOffset, rollOffset);
}
