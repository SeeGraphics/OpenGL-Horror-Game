#ifndef MODEL_HPP
#define MODEL_HPP

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct ModelMesh {
  unsigned int vao = 0;
  unsigned int vbo = 0;
  unsigned int ebo = 0;
  unsigned int texture = 0;
  unsigned int normalMap = 0;
  int indexCount = 0;
};

class Model {
 public:
  Model() = default;
  explicit Model(const char* path);

  void Load(const char* path);
  void Shutdown();

  const std::string& GetPath() const { return modelPath; }
  bool IsLoaded() const { return isLoaded; }
  const std::vector<ModelMesh>& GetMeshes() const { return meshes; }

 private:
  std::string modelPath;
  bool isLoaded = false;
  std::vector<ModelMesh> meshes;
};

struct ModelRenderSettings {
  float albedoIntensity = 1.0f;
  float normalStrength = 1.0f;
  bool normalDebug = false;
  bool useNormalMap = false;
  bool doubleSided = false;
  float alphaCutoff = 0.0f;
};

struct ModelAsset {
  std::string id;
  std::string path;
  Model model;
  ModelRenderSettings renderSettings;
};

struct ModelInstance {
  int assetIndex = -1;
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 rotation = glm::vec3(0.0f);
  glm::vec3 scale = glm::vec3(1.0f);
};

struct ModelTemplate {
  const char* id = nullptr;
  const char* path = nullptr;
  ModelRenderSettings renderSettings;
};

const ModelTemplate* GetModelTemplates(int* count);

#endif
