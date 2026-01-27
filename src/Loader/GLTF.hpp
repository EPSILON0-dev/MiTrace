#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <vector>

#include "Trace/Camera.hpp"
#include "Trace/Light.hpp"
#include "Trace/Material.hpp"
#include "Trace/Mesh.hpp"
#include "Trace/MeshInstance.hpp"
#include "Trace/Scene.hpp"
#include "Trace/TextureImage.hpp"

class GLTF
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
    std::unique_ptr<nlohmann::json> gltfData_;
    std::map<size_t, std::vector<uint8_t>> loadedBuffers_;
    std::map<size_t, std::shared_ptr<Material>> loadedMaterials_;
    std::map<size_t, std::shared_ptr<TextureImage>> loadedImages_;
    std::map<std::pair<size_t, size_t>, std::shared_ptr<Mesh>> loadedMeshes_;

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

   public:  // Rule of five
    GLTF(const std::filesystem::path& filePath);
    ~GLTF();

    GLTF(const GLTF&) = delete;
    GLTF& operator=(const GLTF&) = delete;
    GLTF(GLTF&&) = delete;
    GLTF& operator=(GLTF&&) = delete;

   public:  // Loaders
    using Mesh_sptr = std::shared_ptr<Mesh>;
    using TexImage_sptr = std::shared_ptr<TextureImage>;
    using MeshInstance_vec = std::vector<MeshInstance>;
    using Light_vec = std::vector<Light>;
    using Camera_vec = std::vector<Camera>;
    using Mat4_cref = const glm::mat4&;
    static constexpr glm::mat4 ident = glm::mat4(1.0f);

    Mesh_sptr LoadMesh(size_t meshIndex, size_t primitiveIndex = 0);
    Material LoadMeshMaterial(size_t meshIndex, size_t primitiveIndex = 0);
    TexImage_sptr LoadImage(size_t imageIndex);
    Material LoadMaterial(size_t materialIndex);
    Texture LoadTexture(size_t textureIndex);
    Camera LoadCamera(size_t cameraIndex) const;
    Light LoadLight(size_t lightIndex, Mat4_cref transform = ident);

    MeshInstance_vec LoadNodeMeshes(size_t nodeIndex, Mat4_cref transform = ident);
    MeshInstance_vec LoadSceneMeshes(size_t sceneIndex, Mat4_cref transform = ident);
    Light_vec LoadNodeLights(size_t nodeIndex, Mat4_cref transform = ident);
    Light_vec LoadSceneLights(size_t sceneIndex, Mat4_cref transform = ident);

    Camera_vec LoadNodeCameras(size_t nodeIndex, Mat4_cref transform = ident) const;
    Camera_vec LoadSceneCameras(size_t sceneIndex, Mat4_cref transform = ident) const;
    Camera LoadSceneCamera(size_t sceneIndex, Mat4_cref transform = ident) const;

    std::optional<Texture> LoadSceneEnvironmentTexture();

    Scene LoadScene(size_t sceneIndex = 0, Mat4_cref transform = ident);

   public:  // Other methods
    void Cleanup();
};
