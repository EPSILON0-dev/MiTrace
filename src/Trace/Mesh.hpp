#pragma once

#include <cstddef>
#include <glm/glm.hpp>

class Mesh
{
   private:
    enum class ComponentType
    {
        FLOAT = 5126,
        UNSIGNED_SHORT = 5123,
        UNSIGNED_INT = 5125
    };

    enum class AttributeType
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

    struct BufferFragment
    {
        size_t byteLength;
        size_t byteOffset;
        std::vector<uint8_t> data;
    };

   private:
    std::vector<glm::vec3> positions_;  // REQUIRED
    std::vector<glm::vec3> normals_;    // REQUIRED
    std::vector<glm::vec4> tangents_;   // OPTIONAL
    std::vector<glm::vec2> texCoord0_;  // OPTIONAL (Main texture)
    std::vector<glm::vec2> texCoord1_;  // OPTIONAL (Reserved for future use, e.g. lightmaps)
    std::vector<glm::vec4> color0_;     // OPTIONAL
    std::vector<uint32_t> indices_;     // REQUIRED

   private:
    static bool IsAccessorCorrect(const nlohmann::json& gltf, size_t accessorIndex,
        AttributeType expectedType, ComponentType expectedComponentType);
    static Accessor ParseAccessor(const nlohmann::json& gltf, size_t accessorIndex);
    static BufferView ParseBufferView(const nlohmann::json& gltf, size_t bufferViewIndex);
    static BufferFragment LoadBufferFragment(const nlohmann::json& gltf,
        const std::filesystem::path& basePath, const BufferView& bufferView);
    template <typename T>
    static std::vector<T> LoadAttributeData(const BufferFragment& fragment,
        const Accessor& accessor, AttributeType expectedType, ComponentType expectedComponentType);
    template <typename T>
    static std::vector<T> LoadAttribute(const nlohmann::json& gltf,
        const std::filesystem::path& basePath, size_t accessorIndex, AttributeType expectedType,
        ComponentType expectedComponentType);

   private:
    Mesh() noexcept {}

   public:
    const std::vector<glm::vec3>& GetPositions() const noexcept { return positions_; }
    const std::vector<glm::vec3>& GetNormals() const noexcept { return normals_; }
    const std::vector<glm::vec4>& GetTangents() const noexcept { return tangents_; }
    const std::vector<glm::vec2>& GetTexCoord0() const noexcept { return texCoord0_; }
    const std::vector<glm::vec2>& GetTexCoord1() const noexcept { return texCoord1_; }
    const std::vector<glm::vec4>& GetColor0() const noexcept { return color0_; }
    const std::vector<uint32_t>& GetIndices() const noexcept { return indices_; }

    // TODO: Rework this interface (it sucks)
   public:
    static Mesh FromGLTF(const nlohmann::json& gltf, size_t meshIndex, size_t primitiveIndex,
        const std::filesystem::path& basePath = std::filesystem::current_path());

   public:
    glm::vec3 GetAABBMin() const noexcept;
    glm::vec3 GetAABBMax() const noexcept;
};
