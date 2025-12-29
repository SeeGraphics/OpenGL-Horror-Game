#ifndef WORLD_HPP
#define WORLD_HPP

struct AppState;

void buildFloor(AppState& state);
void buildGrass(AppState& state);
void updateGroundCollision(AppState& state);
float getTerrainHeightAt(const AppState& state, float worldX, float worldZ);

#endif
