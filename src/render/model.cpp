#include "render/model.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glad/glad.h>
#include <stb_image.h>

#include <algorithm>
#include <assimp/Importer.hpp>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "third_party/nlohmann/json.hpp"

using json = nlohmann::json;

// decided to just use models with 1 fbx and 1 texture.png
// -> is much easier to load and works super well with simple
// ps1 style models. Some models need settings.flipUv off to load the texture
// correctly. dont ask me why its flipped by default :P

// SET MODEL SETTINGS
// ------------------------
static ModelRenderSettings makeTreeSettings() {
  // most these arent that importan
  ModelRenderSettings settings;
  settings.flipUv = false;
  settings.alphaCutoff = 0.4f;
  return settings;
}

static ModelRenderSettings makeFlashlightSettings() {
  ModelRenderSettings settings;
  settings.flipUv = false;
  return settings;
}

static ModelRenderSettings makeChurchSettings() {
  ModelRenderSettings settings;
  settings.flipUv = false;
  return settings;
}

static ModelRenderSettings makeWalterSettings() {
  ModelRenderSettings settings;
  settings.flipUv = false;
  return settings;
}

static ModelRenderSettings makeDeadTreeSettings() {
  ModelRenderSettings settings;
  settings.flipUv = false;
  return settings;
}

static ModelRenderSettings makeBarrelSettings() {
  ModelRenderSettings settings;
  settings.flipUv = false;
  return settings;
}

static ModelRenderSettings makeWhiteVanSettings() {
  ModelRenderSettings settings;
  settings.flipUv = false;
  return settings;
}

static ModelRenderSettings makeHandRadioSettings() {
  ModelRenderSettings settings;
  settings.flipUv = false;
  return settings;
}

static ModelRenderSettings makeDeadmanSettings() {
  ModelRenderSettings settings;
  settings.flipUv = false;
  return settings;
}

static ModelRenderSettings makeDeadBodyPlasticbagSettings() {
  ModelRenderSettings settings;
  settings.flipUv = false;
  return settings;
}

static const ModelTemplate gModelTemplates[] = {
    {"Tree", "assets/models/pine_tree/source/pine_tree.fbx", makeTreeSettings(),
     false},
    {"WalterWhite", "assets/models/walter_white/source/Hussainberg.fbx",
     makeWalterSettings(), false},
    {"Church", "assets/models/church/source/church.fbx", makeChurchSettings(),
     true},
    {"Flashlight", "assets/models/flashlight/source/Flashlight.fbx",
     makeFlashlightSettings(), false},
    {"Dead_Tree", "assets/models/dead_tree/source/dead_tree.fbx",
     makeDeadTreeSettings(), false},
    {"metal_barrel", "assets/models/metal_barrel/source/MetalBarrel.fbx",
     makeBarrelSettings(), false},
    {"white_van", "assets/models/white_van/source/white_van.fbx",
     makeWhiteVanSettings(), false},
    {"hand_radio", "assets/models/hand_radio_emissive/scene.gltf",
     makeHandRadioSettings(), false},
    {"deadman", "assets/models/deadman/source/deadman.fbx",
     makeDeadmanSettings(), false},
    {"dead_body_plasticbag",
     "assets/models/dead_body_plasticbag/source/deadbody.fbx",
     makeDeadBodyPlasticbagSettings(), false},
};

const ModelTemplate *GetModelTemplates(int *count) {
  if (count) {
    *count =
        static_cast<int>(sizeof(gModelTemplates) / sizeof(gModelTemplates[0]));
  }
  return gModelTemplates;
}

struct TextureInfo {
  unsigned int id = 0;
  std::string path;
};

static std::string getDirectory(const std::string &path) {
  size_t lastSlash = path.find_last_of("/\\");
  if (lastSlash == std::string::npos) {
    return ".";
  }
  return path.substr(0, lastSlash);
}

static std::string getFileName(const std::string &path) {
  size_t lastSlash = path.find_last_of("/\\");
  if (lastSlash == std::string::npos) {
    return path;
  }
  return path.substr(lastSlash + 1);
}

static std::string joinPath(const std::string &base, const std::string &child) {
  if (child.empty()) {
    return base;
  }
  if (child[0] == '/' || child[0] == '\\') {
    return child;
  }
  return base + "/" + child;
}

static std::string toLower(const std::string &value) {
  std::string out = value;
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

static std::string chooseFallbackBaseName(const aiScene *scene, aiMesh *mesh) {
  std::string meshName;
  if (mesh && mesh->mName.length > 0) {
    meshName = mesh->mName.C_Str();
  }
  std::string materialName;
  if (scene && mesh && mesh->mMaterialIndex < scene->mNumMaterials) {
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    if (material) {
      aiString aiMaterialName;
      if (material->Get(AI_MATKEY_NAME, aiMaterialName) == AI_SUCCESS) {
        if (aiMaterialName.length > 0) {
          materialName = aiMaterialName.C_Str();
        }
      }
    }
  }

  std::string combined = meshName;
  if (!materialName.empty()) {
    if (!combined.empty()) {
      combined += " ";
    }
    combined += materialName;
  }

  std::string lower = toLower(combined);
  if (lower.find("branch") != std::string::npos ||
      lower.find("leaf") != std::string::npos ||
      lower.find("leav") != std::string::npos) {
    return "branches";
  }
  return "tree";
}

static int chooseUvChannel(const aiScene *scene, aiMesh *mesh) {
  int fallback = -1;
  if (mesh) {
    for (int channel = 0; channel < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++channel) {
      if (mesh->HasTextureCoords(channel)) {
        fallback = channel;
        break;
      }
    }
  }

  if (!scene || !mesh) {
    return fallback;
  }
  if (mesh->mMaterialIndex >= scene->mNumMaterials) {
    return fallback;
  }
  aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
  if (!material) {
    return fallback;
  }

  const aiTextureType textureTypes[] = {aiTextureType_DIFFUSE,
                                        aiTextureType_BASE_COLOR};
  for (aiTextureType type : textureTypes) {
    if (material->GetTextureCount(type) == 0) {
      continue;
    }
    aiString texturePath;
    unsigned int uvIndex = 0;
    if (material->GetTexture(type, 0, &texturePath, nullptr, &uvIndex) !=
        AI_SUCCESS) {
      continue;
    }
    if (uvIndex < AI_MAX_NUMBER_OF_TEXTURECOORDS &&
        mesh->HasTextureCoords(uvIndex)) {
      return static_cast<int>(uvIndex);
    }
  }

  return fallback;
}

static unsigned int getDefaultWhiteTexture() {
  static unsigned int textureId = 0;
  if (textureId != 0) {
    return textureId;
  }

  unsigned char whitePixel[3] = {255, 255, 255};
  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE,
               whitePixel);
  return textureId;
}

static unsigned int getDefaultNormalTexture() {
  static unsigned int textureId = 0;
  if (textureId != 0) {
    return textureId;
  }

  unsigned char normalPixel[3] = {128, 128, 255};
  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE,
               normalPixel);
  return textureId;
}

static unsigned int getDefaultEmissiveTexture() {
  static unsigned int textureId = 0;
  if (textureId != 0) {
    return textureId;
  }

  // Default emissive is black (no emission)
  unsigned char blackPixel[3] = {0, 0, 0};
  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE,
               blackPixel);
  return textureId;
}

static unsigned int loadTextureFromFile(const std::string &path) {
  int width = 0;
  int height = 0;
  int channels = 0;

  stbi_set_flip_vertically_on_load(true);
  unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 4);
  stbi_set_flip_vertically_on_load(false);

  if (!data) {
    return 0;
  }

  GLenum format = GL_RGBA;

  unsigned int textureId = 0;
  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
               GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(data);

  return textureId;
}

static TextureInfo loadTextureByBaseName(const std::string &baseName,
                                         const std::string &texturesDirectory) {
  TextureInfo result;
  const char *extensions[] = {".png", ".jpg", ".jpeg"};
  for (const char *ext : extensions) {
    std::string path = joinPath(texturesDirectory, baseName + ext);
    unsigned int textureId = loadTextureFromFile(path);
    if (textureId != 0) {
      result.id = textureId;
      result.path = path;
      break;
    }
  }
  return result;
}

ModelConfig loadModelConfig(const std::string &modelDirectory) {
  ModelConfig config;
  std::string jsonPath = joinPath(modelDirectory, "model.json");
  std::ifstream file(jsonPath);
  if (!file.is_open()) {
    return config;
  }

  try {
    json j = json::parse(file);

    // Parse textureMapping nested object
    if (j.contains("textureMapping") && j["textureMapping"].is_object()) {
      for (auto &[key, value] : j["textureMapping"].items()) {
        config.textureMapping[key] = value.get<std::string>();
      }
    }

    // Parse scale value (optional, defaults to 1.0)
    if (j.contains("scale") && j["scale"].is_number()) {
      config.scale = j["scale"].get<float>();
    }
  } catch (const json::exception &e) {
    std::cout << "JSON parse error in " << jsonPath << ": " << e.what()
              << std::endl;
  }

  return config;
}

bool saveModelConfig(const std::string &modelDirectory,
                     const ModelConfig &config) {
  std::string jsonPath = joinPath(modelDirectory, "model.json");
  std::ofstream file(jsonPath);
  if (!file.is_open()) {
    std::cout << "Failed to open " << jsonPath << " for writing" << std::endl;
    return false;
  }

  try {
    json j;
    // Save textureMapping nested object
    j["textureMapping"] = json::object();
    for (const auto &[key, value] : config.textureMapping) {
      j["textureMapping"][key] = value;
    }
    // Save scale value
    j["scale"] = config.scale;

    // Write with pretty formatting (2 space indent)
    file << j.dump(2) << std::endl;
    return true;
  } catch (const json::exception &e) {
    std::cout << "JSON write error in " << jsonPath << ": " << e.what()
              << std::endl;
    return false;
  }
}

static TextureInfo loadMaterialTexture(const aiScene *scene, aiMesh *mesh,
                                       const std::string &modelDirectory,
                                       const std::string &texturesDirectory,
                                       aiTextureType type) {
  TextureInfo result;
  if (!scene || !mesh) {
    return result;
  }
  if (mesh->mMaterialIndex >= scene->mNumMaterials) {
    return result;
  }

  aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
  if (!material) {
    return result;
  }

  if (material->GetTextureCount(type) == 0) {
    return result;
  }

  aiString texturePath;
  if (material->GetTexture(type, 0, &texturePath) != AI_SUCCESS) {
    return result;
  }

  std::string pathValue = texturePath.C_Str();
  std::string fileName = getFileName(pathValue);

  // Try direct path first (relative to model directory)
  std::string directPath = joinPath(modelDirectory, pathValue);
  unsigned int directId = loadTextureFromFile(directPath);
  if (directId != 0) {
    result.id = directId;
    result.path = directPath;
    return result;
  }

  // Try absolute path if direct path failed
  unsigned int absoluteId = loadTextureFromFile(pathValue);
  if (absoluteId != 0) {
    result.id = absoluteId;
    result.path = pathValue;
    return result;
  }

  // Try in textures directory
  std::string texturesPath = joinPath(texturesDirectory, fileName);
  unsigned int texturesId = loadTextureFromFile(texturesPath);
  if (texturesId != 0) {
    result.id = texturesId;
    result.path = texturesPath;
    return result;
  }

  return result;
}

static std::string getMaterialName(const aiScene *scene, aiMesh *mesh) {
  if (!scene || !mesh) {
    return "";
  }
  if (mesh->mMaterialIndex >= scene->mNumMaterials) {
    return "";
  }

  aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
  if (!material) {
    return "";
  }

  aiString materialName;
  if (material->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS) {
    if (materialName.length > 0) {
      return std::string(materialName.C_Str());
    }
  }

  return "";
}

static TextureInfo
loadDiffuseTexture(const aiScene *scene, aiMesh *mesh,
                   const std::string &modelDirectory,
                   const std::string &texturesDirectory,
                   const std::map<std::string, std::string> &textureMapping) {
  std::string materialName = getMaterialName(scene, mesh);
  TextureInfo result;

  // First, try to load texture directly from material (works for glTF models)
  result = loadMaterialTexture(scene, mesh, modelDirectory, texturesDirectory,
                               aiTextureType_DIFFUSE);
  if (result.id != 0) {
    return result;
  }
  // Try BASE_COLOR for glTF PBR materials
  result = loadMaterialTexture(scene, mesh, modelDirectory, texturesDirectory,
                               aiTextureType_BASE_COLOR);
  if (result.id != 0) {
    return result;
  }

  // Fallback to JSON texture mapping (for FBX models)
  if (!textureMapping.empty() && !materialName.empty()) {
    auto it = textureMapping.find(materialName);
    if (it != textureMapping.end()) {
      result = loadTextureByBaseName(it->second, texturesDirectory);
      if (result.id != 0) {
        return result;
      } else {
        // Mapping exists but texture file not found
        std::cout << "Warning: Material '" << materialName
                  << "' mapped to texture '" << it->second
                  << "' but file not found in " << texturesDirectory
                  << std::endl;
      }
    }
  }

  // Fallback to default white texture if all methods fail
  result.id = getDefaultWhiteTexture();

  return result;
}

static unsigned int loadNormalTexture(const aiScene *scene, aiMesh *mesh,
                                      const std::string &modelDirectory,
                                      const std::string &texturesDirectory,
                                      const std::string &diffusePath) {
  TextureInfo result = loadMaterialTexture(
      scene, mesh, modelDirectory, texturesDirectory, aiTextureType_NORMALS);
  if (result.id == 0) {
    result = loadMaterialTexture(scene, mesh, modelDirectory, texturesDirectory,
                                 aiTextureType_HEIGHT);
  }
  if (result.id != 0) {
    return result.id;
  }

  if (!diffusePath.empty()) {
    std::string normalPath = diffusePath;
    size_t token = normalPath.find("Color");
    if (token != std::string::npos) {
      normalPath.replace(token, std::strlen("Color"), "Normal");
      unsigned int normalId = loadTextureFromFile(normalPath);
      if (normalId != 0) {
        return normalId;
      }
    }
  }

  std::string baseName = chooseFallbackBaseName(scene, mesh);
  std::string fallbackPath =
      joinPath(texturesDirectory, baseName + "Normal.png");
  unsigned int fallbackId = loadTextureFromFile(fallbackPath);
  if (fallbackId != 0) {
    return fallbackId;
  }

  return getDefaultNormalTexture();
}

static unsigned int loadEmissiveTexture(const aiScene *scene, aiMesh *mesh,
                                        const std::string &modelDirectory,
                                        const std::string &texturesDirectory) {
  // Try to load emissive texture from material
  TextureInfo result = loadMaterialTexture(
      scene, mesh, modelDirectory, texturesDirectory, aiTextureType_EMISSIVE);
  if (result.id != 0) {
    return result.id;
  }

  // No emissive texture found, return default black texture
  return getDefaultEmissiveTexture();
}

Model::Model(const char *path) { Load(path); }

void Model::Shutdown() {
  unsigned int defaultDiffuse = getDefaultWhiteTexture();
  unsigned int defaultNormal = getDefaultNormalTexture();
  unsigned int defaultEmissive = getDefaultEmissiveTexture();
  for (ModelMesh &mesh : meshes) {
    if (mesh.vao)
      glDeleteVertexArrays(1, &mesh.vao);
    if (mesh.vbo)
      glDeleteBuffers(1, &mesh.vbo);
    if (mesh.ebo)
      glDeleteBuffers(1, &mesh.ebo);
    if (mesh.texture && mesh.texture != defaultDiffuse)
      glDeleteTextures(1, &mesh.texture);
    if (mesh.normalMap && mesh.normalMap != defaultNormal)
      glDeleteTextures(1, &mesh.normalMap);
    if (mesh.emissiveMap && mesh.emissiveMap != defaultEmissive)
      glDeleteTextures(1, &mesh.emissiveMap);
  }
  meshes.clear();
  isLoaded = false;
  boundingRadius = 0.0f;
  baseScale = 1.0f;
}

void Model::Load(const char *path) {
  ModelRenderSettings settings;
  Load(path, settings);
}

void Model::Load(const char *path, const ModelRenderSettings &settings) {
  Shutdown();

  if (path) {
    modelPath = path;
  } else {
    modelPath.clear();
  }

  if (!path || modelPath.empty()) {
    return;
  }

  Assimp::Importer importer;
  unsigned int flags = aiProcess_Triangulate | aiProcess_CalcTangentSpace |
                       aiProcess_GenSmoothNormals |
                       aiProcess_PreTransformVertices;
  if (settings.flipUv) {
    flags |= aiProcess_FlipUVs;
  }
  const aiScene *scene = importer.ReadFile(modelPath, flags);
  if (!scene || !scene->mRootNode || scene->mNumMeshes == 0) {
    std::cout << "Model loading failed" << std::endl;
    return;
  }

  std::string modelDirectory = getDirectory(modelPath);
  std::string texturesDirectory =
      joinPath(getDirectory(modelDirectory), "textures");

  // Load model configuration from JSON file
  std::string modelRootDirectory = getDirectory(modelDirectory);
  ModelConfig modelConfig = loadModelConfig(modelRootDirectory);
  const std::map<std::string, std::string> &textureMapping =
      modelConfig.textureMapping;
  baseScale = modelConfig.scale;

  meshes.reserve(scene->mNumMeshes);
  float maxRadiusSq = 0.0f;
  for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
    aiMesh *mesh = scene->mMeshes[meshIndex];
    if (!mesh)
      continue;

    int uvChannel = chooseUvChannel(scene, mesh);

    std::vector<float> vertices;
    vertices.reserve(mesh->mNumVertices * 11);

    for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
      aiVector3D pos = mesh->mVertices[v];
      // Apply scale from model.json to vertex positions
      pos.x *= baseScale;
      pos.y *= baseScale;
      pos.z *= baseScale;
      float radiusSq = (pos.x * pos.x) + (pos.y * pos.y) + (pos.z * pos.z);
      if (radiusSq > maxRadiusSq) {
        maxRadiusSq = radiusSq;
      }
      aiVector3D normal =
          mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0.0f, 1.0f, 0.0f);
      aiVector3D texCoord = (uvChannel >= 0)
                                ? mesh->mTextureCoords[uvChannel][v]
                                : aiVector3D(0.0f, 0.0f, 0.0f);
      aiVector3D tangent = (mesh->HasTangentsAndBitangents())
                               ? mesh->mTangents[v]
                               : aiVector3D(1.0f, 0.0f, 0.0f);

      vertices.push_back(pos.x);
      vertices.push_back(pos.y);
      vertices.push_back(pos.z);

      vertices.push_back(normal.x);
      vertices.push_back(normal.y);
      vertices.push_back(normal.z);

      vertices.push_back(texCoord.x);
      vertices.push_back(texCoord.y);

      vertices.push_back(tangent.x);
      vertices.push_back(tangent.y);
      vertices.push_back(tangent.z);
    }

    std::vector<unsigned int> indices;
    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
      aiFace face = mesh->mFaces[f];
      for (unsigned int i = 0; i < face.mNumIndices; ++i) {
        indices.push_back(face.mIndices[i]);
      }
    }

    TextureInfo diffuse = loadDiffuseTexture(scene, mesh, modelDirectory,
                                             texturesDirectory, textureMapping);
    unsigned int normalMap = loadNormalTexture(scene, mesh, modelDirectory,
                                               texturesDirectory, diffuse.path);
    unsigned int emissiveMap =
        loadEmissiveTexture(scene, mesh, modelDirectory, texturesDirectory);

    ModelMesh outMesh;
    outMesh.indexCount = static_cast<int>(indices.size());
    outMesh.texture = diffuse.id;
    outMesh.normalMap = normalMap;
    outMesh.emissiveMap = emissiveMap;

    glGenVertexArrays(1, &outMesh.vao);
    glGenBuffers(1, &outMesh.vbo);
    glGenBuffers(1, &outMesh.ebo);
    glBindVertexArray(outMesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, outMesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outMesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float),
                          (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float),
                          (void *)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);

    meshes.push_back(outMesh);
  }

  isLoaded = !meshes.empty();
  boundingRadius = (maxRadiusSq > 0.0f) ? std::sqrt(maxRadiusSq) : 0.0f;
  if (isLoaded) {
    std::cout << "Model loaded: " << modelPath << std::endl;
  }
}
