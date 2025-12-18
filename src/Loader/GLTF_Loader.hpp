#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

#include "Trace/Image.hpp"
#include "Trace/Light.hpp"
#include "Trace/MaterialGLTF.hpp"
#include "Trace/Mesh.hpp"
#include "Trace/MeshInstance.hpp"

class GLTF_Loader
{
   private:  // Mesh related types
    enum class ComponentType : uint16_t
    {
        FLOAT = 5126,
        UNSIGNED_SHORT = 5123,
        UNSIGNED_INT = 5125
    };

    enum class AttributeType : uint8_t
    {
        SCALAR,
        VEC2,
        VEC3,
        VEC4
    };

    struct Accessor
    {
        size_t bufferViewIndex;
        size_t byteOffset;
        size_t count;
        ComponentType componentType;
        AttributeType attributeType;
        size_t componentSize;
        size_t attributeCount;
    };

    struct BufferView
    {
        size_t bufferIndex;
        size_t byteOffset;
        size_t byteLength;
        size_t byteStride;
    };

   private:  // General GLTF fields
    std::filesystem::path filePath_;
    std::filesystem::path basePath_;
    nlohmann::json gltfData_;
    std::map<size_t, std::vector<uint8_t>> loadedBuffers_;
    std::map<size_t, std::shared_ptr<MaterialGLTF>> loadedMaterials_;
    std::map<size_t, std::shared_ptr<Image>> loadedImages_;
    std::map<size_t, std::shared_ptr<Mesh>> loadedMeshes_;

   private:  // Helper methods
    const std::vector<uint8_t>& GetBufferData(size_t bufferIndex);
    bool IsAccessorCorrect(
        size_t accessorIndex, AttributeType expectedType, ComponentType expectedComponentType);
    Accessor ParseAccessor(size_t accessorIndex);
    BufferView ParseBufferView(size_t bufferViewIndex);
    template <typename T>
    std::vector<T> LoadMeshAttributeData(const BufferView& bufferView, const Accessor& accessor,
        AttributeType expectedType, ComponentType expectedComponentType);
    template <typename T>
    std::vector<T> LoadMeshAttribute(
        size_t accessorIndex, AttributeType expectedType, ComponentType expectedComponentType);
    glm::mat4 ComputeNodeTransform(size_t nodeIndex) const;

    Light::PointLight LoadPointLight(size_t lightIndex) const;
    Light::DirectionalLight LoadDirectionalLight(size_t lightIndex) const;
    Light::SpotLight LoadSpotLight(size_t lightIndex) const;
    Light::AreaLight LoadAreaLight(size_t lightIndex) const;

   public:  // Rule of zero
    GLTF_Loader(const std::filesystem::path& filePath);

   public:  // Loaders
    std::shared_ptr<Mesh> LoadMesh(size_t meshIndex, size_t primitiveIndex = 0);
    std::shared_ptr<Image> LoadImage(size_t imageIndex);
    Texture LoadTexture(size_t textureIndex);
    std::shared_ptr<MaterialGLTF> LoadMaterial(size_t materialIndex);

    Light LoadLight(size_t lightIndex, const glm::mat4& transform = glm::mat4(1.0f));

    std::vector<MeshInstance> LoadNodeMeshes(
        size_t nodeIndex, const glm::mat4& transform = glm::mat4(1.0f));
    std::vector<MeshInstance> LoadSceneMeshes(
        size_t sceneIndex, const glm::mat4& transform = glm::mat4(1.0f));

    std::vector<Light> LoadNodeLights(
        size_t nodeIndex, const glm::mat4& transform = glm::mat4(1.0f));
    std::vector<Light> LoadSceneLights(
        size_t sceneIndex, const glm::mat4& transform = glm::mat4(1.0f));

   public:  // Other methods
    void Cleanup();
};
