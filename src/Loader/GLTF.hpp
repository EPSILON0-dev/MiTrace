#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <vector>

#include "Types.hpp"

namespace Loader
{
class GLTF
{
   private:  // Mesh related types
    enum class ComponentType : uint16_t
    {
        Float = 5126,
        UnsignedShort = 5123,
        UnsignedInt = 5125
    };

    enum class AttributeType : uint8_t
    {
        Scalar,
        Vec2,
        Vec3,
        Vec4
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
    std::map<std::pair<size_t, size_t>, std::shared_ptr<Mesh>> loadedMeshes_;

   private:  // Helper methods
    static Material GenerateDefaultMaterial();
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
    std::vector<uint32_t> LoadMeshIndices(size_t accessor);
    glm::mat4 ComputeNodeTransform(size_t nodeIndex) const;
    Texture LoadTextureInfo(const nlohmann::json& textureInfoData);

    Light LoadPointLight(size_t lightIndex) const;
    Light LoadDirectionalLight(size_t lightIndex) const;
    Light LoadSpotLight(size_t lightIndex) const;
    Light LoadAreaLight(size_t areaLightIndex) const;

   public:  // Rule of five
    GLTF(const std::filesystem::path& filePath);
    ~GLTF();

    GLTF(const GLTF&) = delete;
    GLTF& operator=(const GLTF&) = delete;
    GLTF(GLTF&&) = delete;
    GLTF& operator=(GLTF&&) = delete;

   public:  // Loaders
    using Mesh_sptr = std::shared_ptr<Mesh>;
    using MeshInstance_vec = std::vector<MeshInstance>;
    using Light_vec = std::vector<Light>;
    using Camera_vec = std::vector<Camera>;
    using Mat4_cref = const glm::mat4&;
    static constexpr glm::mat4 ident = glm::mat4(1.0f);

    Mesh_sptr LoadMesh(size_t meshIndex, size_t primitiveIndex = 0);
    Material LoadMeshMaterial(size_t meshIndex, size_t primitiveIndex = 0);
    Image LoadImage(size_t imageIndex);
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
    Camera LoadSceneCamera(size_t sceneIndex, size_t cameraIndex, Mat4_cref transform = ident) const;

    std::optional<Texture> LoadSceneEnvironmentTexture();

    Scene LoadScene(size_t sceneIndex = 0, Mat4_cref transform = ident);

   public:  // Other methods
    void Cleanup();
};
}  // namespace Loader
