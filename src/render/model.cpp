#include "render/model.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glad/glad.h>
#include <stb_image.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

static ModelRenderSettings makeTreeSettings() {
  ModelRenderSettings settings;
  settings.albedoIntensity = 1.0f;
  settings.normalStrength = 1.0f;
  settings.normalDebug = false;
  settings.useNormalMap = true;
  settings.doubleSided = true;
  settings.alphaCutoff = 0.4f;
  return settings;
}

static const ModelTemplate gModelTemplates[] = {
    {"Tree", "assets/models/tree/source/tree.fbx", makeTreeSettings()},
};

const ModelTemplate* GetModelTemplates(int* count) {
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

static std::string getDirectory(const std::string& path) {
  size_t lastSlash = path.find_last_of("/\\");
  if (lastSlash == std::string::npos) {
    return ".";
  }
  return path.substr(0, lastSlash);
}

static std::string getFileName(const std::string& path) {
  size_t lastSlash = path.find_last_of("/\\");
  if (lastSlash == std::string::npos) {
    return path;
  }
  return path.substr(lastSlash + 1);
}

static std::string joinPath(const std::string& base,
                            const std::string& child) {
  if (child.empty()) {
    return base;
  }
  if (child[0] == '/' || child[0] == '\\') {
    return child;
  }
  return base + "/" + child;
}

static std::string toLower(const std::string& value) {
  std::string out = value;
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return out;
}

static std::string chooseFallbackBaseName(const aiScene* scene, aiMesh* mesh) {
  std::string meshName;
  if (mesh && mesh->mName.length > 0) {
    meshName = mesh->mName.C_Str();
  }
  std::string materialName;
  if (scene && mesh && mesh->mMaterialIndex < scene->mNumMaterials) {
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    if (material) {
      aiString materialName;
      if (material->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS) {
        if (materialName.length > 0) {
          materialName = materialName.C_Str();
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

static unsigned int loadTextureFromFile(const std::string& path) {
  int width = 0;
  int height = 0;
  int channels = 0;

  stbi_set_flip_vertically_on_load(true);
  unsigned char* data =
      stbi_load(path.c_str(), &width, &height, &channels, 4);
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

static TextureInfo loadTextureByBaseName(
    const std::string& baseName,
    const std::string& texturesDirectory) {
  TextureInfo result;
  const char* extensions[] = {".png", ".jpg", ".jpeg"};
  for (const char* ext : extensions) {
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

static TextureInfo loadMaterialTexture(const aiScene* scene, aiMesh* mesh,
                                       const std::string& modelDirectory,
                                       const std::string& texturesDirectory,
                                       aiTextureType type) {
  TextureInfo result;
  if (!scene || !mesh) {
    return result;
  }
  if (mesh->mMaterialIndex >= scene->mNumMaterials) {
    return result;
  }

  aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
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

  std::string directPath = joinPath(modelDirectory, pathValue);
  unsigned int directId = loadTextureFromFile(directPath);
  if (directId != 0) {
    result.id = directId;
    result.path = directPath;
    return result;
  }

  std::string texturesPath = joinPath(texturesDirectory, fileName);
  unsigned int texturesId = loadTextureFromFile(texturesPath);
  if (texturesId != 0) {
    result.id = texturesId;
    result.path = texturesPath;
    return result;
  }

  return result;
}

static TextureInfo loadDiffuseTexture(const aiScene* scene, aiMesh* mesh,
                                      const std::string& modelDirectory,
                                      const std::string& texturesDirectory) {
  std::string baseName = chooseFallbackBaseName(scene, mesh);
  if (baseName == "branches") {
    TextureInfo forced = loadTextureByBaseName("branches", texturesDirectory);
    if (forced.id != 0) {
      return forced;
    }
  }

  TextureInfo result =
      loadMaterialTexture(scene, mesh, modelDirectory, texturesDirectory,
                          aiTextureType_DIFFUSE);
  if (result.id == 0) {
    result =
        loadMaterialTexture(scene, mesh, modelDirectory, texturesDirectory,
                            aiTextureType_BASE_COLOR);
  }
  if (result.id == 0) {
    result = loadTextureByBaseName(baseName, texturesDirectory);
  }

  if (result.id == 0) {
    result.id = getDefaultWhiteTexture();
  }
  return result;
}

static unsigned int loadNormalTexture(const aiScene* scene, aiMesh* mesh,
                                      const std::string& modelDirectory,
                                      const std::string& texturesDirectory,
                                      const std::string& diffusePath) {
  TextureInfo result =
      loadMaterialTexture(scene, mesh, modelDirectory, texturesDirectory,
                          aiTextureType_NORMALS);
  if (result.id == 0) {
    result =
        loadMaterialTexture(scene, mesh, modelDirectory, texturesDirectory,
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

Model::Model(const char* path) {
  Load(path);
}

void Model::Shutdown() {
  unsigned int defaultDiffuse = getDefaultWhiteTexture();
  unsigned int defaultNormal = getDefaultNormalTexture();
  for (ModelMesh& mesh : meshes) {
    if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
    if (mesh.vbo) glDeleteBuffers(1, &mesh.vbo);
    if (mesh.ebo) glDeleteBuffers(1, &mesh.ebo);
    if (mesh.texture && mesh.texture != defaultDiffuse)
      glDeleteTextures(1, &mesh.texture);
    if (mesh.normalMap && mesh.normalMap != defaultNormal)
      glDeleteTextures(1, &mesh.normalMap);
  }
  meshes.clear();
  isLoaded = false;
  boundingRadius = 0.0f;
}

void Model::Load(const char* path) {
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
  const aiScene* scene = importer.ReadFile(
      modelPath, aiProcess_Triangulate | aiProcess_FlipUVs |
                     aiProcess_CalcTangentSpace |
                     aiProcess_GenSmoothNormals |
                     aiProcess_PreTransformVertices);
  if (!scene || !scene->mRootNode || scene->mNumMeshes == 0) {
    std::cout << "Model loading failed" << std::endl;
    return;
  }

  std::string modelDirectory = getDirectory(modelPath);
  std::string texturesDirectory =
      joinPath(getDirectory(modelDirectory), "textures");

  meshes.reserve(scene->mNumMeshes);
  float maxRadiusSq = 0.0f;
  for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
    aiMesh* mesh = scene->mMeshes[meshIndex];
    if (!mesh) continue;

    int uvChannel = -1;
    for (int channel = 0; channel < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++channel) {
      if (mesh->HasTextureCoords(channel)) {
        uvChannel = channel;
        break;
      }
    }

    std::vector<float> vertices;
    vertices.reserve(mesh->mNumVertices * 11);

    for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
      aiVector3D pos = mesh->mVertices[v];
      float radiusSq = (pos.x * pos.x) + (pos.y * pos.y) + (pos.z * pos.z);
      if (radiusSq > maxRadiusSq) {
        maxRadiusSq = radiusSq;
      }
      aiVector3D normal =
          mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0.0f, 1.0f, 0.0f);
      aiVector3D texCoord =
          (uvChannel >= 0) ? mesh->mTextureCoords[uvChannel][v]
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

    TextureInfo diffuse =
        loadDiffuseTexture(scene, mesh, modelDirectory, texturesDirectory);
    unsigned int normalMap = loadNormalTexture(scene, mesh, modelDirectory,
                                               texturesDirectory, diffuse.path);

    ModelMesh outMesh;
    outMesh.indexCount = static_cast<int>(indices.size());
    outMesh.texture = diffuse.id;
    outMesh.normalMap = normalMap;

    glGenVertexArrays(1, &outMesh.vao);
    glGenBuffers(1, &outMesh.vbo);
    glGenBuffers(1, &outMesh.ebo);
    glBindVertexArray(outMesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, outMesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outMesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int), indices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float),
                          (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float),
                          (void*)(8 * sizeof(float)));
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
