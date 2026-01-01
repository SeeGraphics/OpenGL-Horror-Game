#ifndef WORLD_HPP
#define WORLD_HPP

#include <glm/glm.hpp>

struct AppState;

void buildFloor(AppState& state);
void buildGrass(AppState& state);
void updateGroundCollision(AppState& state);
float getTerrainHeightAt(const AppState& state, float worldX, float worldZ);
void initWorldModels(AppState& state);
void updateWorldModelHeights(AppState& state);
int addModelInstance(AppState& state, int assetIndex, const glm::vec3& position,
                     const glm::vec3& rotation, const glm::vec3& scale);
void rebuildWorldTrees(AppState& state);
void updateFlashlightAttachment(AppState& state);

#endif
