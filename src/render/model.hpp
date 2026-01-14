#ifndef MODEL_HPP
#define MODEL_HPP

#include <glm/glm.hpp>
#include <map>
#include <string>
#include <vector>

struct ModelRenderSettings;

struct ModelMesh {
  unsigned int vao = 0;
  unsigned int vbo = 0;
  unsigned int ebo = 0;
  unsigned int texture = 0;
  unsigned int normalMap = 0;
  unsigned int emissiveMap = 0;
  int indexCount = 0;
};

// json structure, map for texture mapping and model scale
struct ModelConfig {
  std::map<std::string, std::string> textureMapping;
  float scale = 1.0f;
};

class Model {
public:
  Model() = default;
  explicit Model(const char *path);

  void Load(const char *path);
  void Load(const char *path, const ModelRenderSettings &settings);
  void Shutdown();

  const std::string &GetPath() const { return modelPath; }
  bool IsLoaded() const { return isLoaded; }
  const std::vector<ModelMesh> &GetMeshes() const { return meshes; }
  float GetBoundingRadius() const { return boundingRadius; }
  float GetBaseScale() const { return baseScale; }

private:
  std::string modelPath;
  bool isLoaded = false;
  std::vector<ModelMesh> meshes;
  float boundingRadius = 0.0f;
  float baseScale = 1.0f;
};

struct ModelRenderSettings {
  float albedoIntensity = 1.0f;
  float normalStrength = 1.0f;
  bool normalDebug = false;
  bool useNormalMap = false;
  bool doubleSided = false;
  float alphaCutoff = 0.0f;
  bool flipUv = true;
};

struct ModelAsset {
  std::string id;
  std::string path;
  Model model;
  ModelRenderSettings renderSettings;
  bool freeArea = false;
};

struct ModelInstance {
  int assetIndex = -1;
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 rotation = glm::vec3(0.0f);
  glm::vec3 scale = glm::vec3(1.0f);
  bool isEditorPlaced = false;
  bool freeArea = false;
  float freeAreaRadius = 0.0f;
};

struct ModelTemplate {
  const char *id = nullptr;
  const char *path = nullptr;
  ModelRenderSettings renderSettings;
  bool freeArea = false;
};

const ModelTemplate *GetModelTemplates(int *count);

// Load and save model configuration from/to model.json
ModelConfig loadModelConfig(const std::string &modelDirectory);
bool saveModelConfig(const std::string &modelDirectory,
                     const ModelConfig &config);

#endif
