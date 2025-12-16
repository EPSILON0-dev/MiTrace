#include "Mesh.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdexcept>

Mesh::Accessor Mesh::ParseAccessor(const nlohmann::json& gltf, size_t accessorIndex)
{
    const auto& accessorData = gltf["accessors"][accessorIndex];

    Accessor accessor;
    accessor.bufferViewIndex = accessorData["bufferView"].get<size_t>();
    accessor.byteOffset = accessorData.value("byteOffset", 0);
    accessor.count = accessorData["count"].get<size_t>();

    size_t componentType = accessorData["componentType"].get<size_t>();
    auto attributeType = accessorData["type"].get<std::string>();

    static const std::map<std::string, AttributeType> typeComponentCounts = {
        {"SCALAR", AttributeType::SCALAR},                                           // SCALAR
        {"VEC2", AttributeType::VEC2},                                               // VEC2
        {"VEC3", AttributeType::VEC3},                                               // VEC3
        {"VEC4", AttributeType::VEC4}};                                              // VEC4
    static const std::map<std::string, size_t> typeComponentSizes = {{"SCALAR", 1},  // SCALAR
        {"VEC2", 2},                                                                 // VEC2
        {"VEC3", 3},                                                                 // VEC3
        {"VEC4", 4}};                                                                // VEC4
    try
    {
        accessor.attributeType = typeComponentCounts.at(attributeType);
        accessor.attributeCount = typeComponentSizes.at(attributeType);
    }
    catch (const std::out_of_range&)
    {
        throw std::runtime_error("Unsupported attribute type for loading data");
    }

    static const std::map<size_t, ComponentType> componentTypes = {
        {5126, ComponentType::FLOAT},           // FLOAT
        {5123, ComponentType::UNSIGNED_SHORT},  // UNSIGNED SHORT
        {5125, ComponentType::UNSIGNED_INT}     // UNSIGNED INT
    };
    static const std::map<size_t, size_t> componentSizes = {
        {5126, sizeof(float)},     // FLOAT
        {5123, sizeof(uint16_t)},  // UNSIGNED SHORT
        {5125, sizeof(uint32_t)}   // UNSIGNED INT
    };
    try
    {
        accessor.componentType = componentTypes.at(componentType);
        accessor.componentSize = componentSizes.at(componentType);
    }
    catch (const std::out_of_range&)
    {
        throw std::runtime_error("Unsupported component type for attribute data");
    }

    return accessor;
}

Mesh::BufferView Mesh::ParseBufferView(const nlohmann::json& gltf, size_t bufferViewIndex)
{
    const auto& bufferViewData = gltf["bufferViews"][bufferViewIndex];

    BufferView bufferView;
    bufferView.bufferIndex = bufferViewData["buffer"].get<size_t>();
    bufferView.byteOffset = bufferViewData.value("byteOffset", 0);
    bufferView.byteLength = bufferViewData["byteLength"].get<size_t>();
    bufferView.byteStride = bufferViewData.value("byteStride", 0);

    return bufferView;
}

Mesh::BufferFragment Mesh::LoadBufferFragment(const nlohmann::json& gltf,
    const std::filesystem::path& basePath, const Mesh::BufferView& bufferView)
{
    const auto& bufferData = gltf["buffers"][bufferView.bufferIndex];
    if (bufferData["uri"].get<std::string>().rfind("data:", 0) == 0)
        throw std::runtime_error("Data URI buffers are not supported");

    BufferFragment fragment;
    fragment.byteOffset = bufferView.byteOffset;
    fragment.byteLength = bufferView.byteLength;
    fragment.data.resize(fragment.byteLength);

    std::filesystem::path bufferPath = basePath / bufferData["uri"].get<std::string>();
    std::ifstream bufferFile(bufferPath, std::ios::binary);
    if (!bufferFile)
        throw std::runtime_error(
            std::format("Failed to open buffer file: {}", bufferPath.string()));

    bufferFile.seekg(fragment.byteOffset, std::ios::beg);
    bufferFile.read(reinterpret_cast<char*>(fragment.data.data()), fragment.byteLength);

    return fragment;
}

template <typename T>
std::vector<T> Mesh::LoadAttributeData(const BufferFragment& fragment, const Accessor& accessor,
    AttributeType expectedType, ComponentType expectedComponentType)
{
    if (accessor.attributeType != expectedType || accessor.componentType != expectedComponentType)
        throw std::runtime_error("Attribute type does not match expected type");

    std::vector<T> data;
    data.reserve(accessor.count);

    // We assume tightly packed data with the same byte order as the host architecture
    size_t stride = accessor.attributeCount * accessor.componentSize;
    for (size_t i = 0; i < accessor.count; ++i)
    {
        size_t offset = accessor.byteOffset + i * stride;
        T value;
        std::memcpy(&value, fragment.data.data() + offset, sizeof(T));
        data.push_back(value);
    }

    return data;
}

bool Mesh::IsAccessorCorrect(const nlohmann::json& gltf, size_t accessorIndex,
    AttributeType expectedType, ComponentType expectedComponentType)
{
    Accessor accessor = ParseAccessor(gltf, accessorIndex);
    return accessor.attributeType == expectedType &&
           accessor.componentType == expectedComponentType;
}

template <typename T>
std::vector<T> Mesh::LoadAttribute(const nlohmann::json& gltf,
    const std::filesystem::path& basePath, size_t accessorIndex, AttributeType expectedType,
    ComponentType expectedComponentType)
{
    Accessor accessor = ParseAccessor(gltf, accessorIndex);

    // If we perform this check here, we can skip unnecessary buffer loading
    if (accessor.attributeType != expectedType || accessor.componentType != expectedComponentType)
        throw std::runtime_error("Accessor type does not match expected type");

    BufferView bufferView = ParseBufferView(gltf, accessor.bufferViewIndex);
    BufferFragment fragment = LoadBufferFragment(gltf, basePath, bufferView);
    return LoadAttributeData<T>(fragment, accessor, expectedType, expectedComponentType);
}

Mesh Mesh::FromGLTF(const nlohmann::json& gltf, size_t meshIndex, size_t primitiveIndex,
    const std::filesystem::path& basePath)
{
    std::string meshName = gltf["meshes"][meshIndex].value("name", "UnnamedMesh");
    Mesh mesh;

    // Validate primitive data
    const auto& primitiveData = gltf["meshes"][meshIndex]["primitives"][primitiveIndex];
    if (!primitiveData.contains("attributes") || !primitiveData.contains("indices"))
        throw std::runtime_error("Invalid GLTF primitive data: missing attributes or indices");
    if (primitiveData["mode"] != 4)
        throw std::runtime_error("Only triangle primitives (mode 4) are supported");
    const auto& attributes = primitiveData["attributes"];
    if (!attributes.contains("POSITION") || !attributes.contains("NORMAL"))
        throw std::runtime_error("POSITION and NORMAL attributes are required");

    // Load indices
    try
    {
        size_t indicesAcc = primitiveData["indices"].get<size_t>();
        bool areIndicesUnsignedInt =
            IsAccessorCorrect(gltf, indicesAcc, AttributeType::SCALAR, ComponentType::UNSIGNED_INT);
        bool areIndicesUnsignedShort = IsAccessorCorrect(
            gltf, indicesAcc, AttributeType::SCALAR, ComponentType::UNSIGNED_SHORT);

        if (!areIndicesUnsignedInt && !areIndicesUnsignedShort)
            throw std::runtime_error("Indices accessor has unsupported component type");

        if (areIndicesUnsignedInt)
        {
            mesh.indices_ = LoadAttribute<uint32_t>(
                gltf, basePath, indicesAcc, AttributeType::SCALAR, ComponentType::UNSIGNED_INT);
        }
        else
        {
            auto shortIndices = LoadAttribute<uint16_t>(
                gltf, basePath, indicesAcc, AttributeType::SCALAR, ComponentType::UNSIGNED_SHORT);
            std::transform(shortIndices.begin(), shortIndices.end(),
                std::back_inserter(mesh.indices_),
                [](uint16_t index) { return static_cast<uint32_t>(index); });
        }
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::format("Failed to load INDICES: {}", e.what()));
    }

    struct
    {
        std::string name;
        AttributeType type;
        ComponentType componentType;
        bool isRequired;
    } loadedAttributes[] = {
        {"POSITION", AttributeType::VEC3, ComponentType::FLOAT, true},     // Required Vector3
        {"NORMAL", AttributeType::VEC3, ComponentType::FLOAT, true},       // Required Vector3
        {"TANGENT", AttributeType::VEC4, ComponentType::FLOAT, false},     // Optional Vector4
        {"TEXCOORD_0", AttributeType::VEC2, ComponentType::FLOAT, false},  // Optional Vector2
        {"TEXCOORD_1", AttributeType::VEC2, ComponentType::FLOAT, false},  // Optional Vector2
        {"COLOR_0", AttributeType::VEC4, ComponentType::FLOAT, false}      // Optional Vector4
    };

    for (const auto& attribute : loadedAttributes)
    {
        if (!attributes.contains(attribute.name))
        {
            if (!attribute.isRequired) continue;
            throw std::runtime_error(std::format("Missing required attribute: {}", attribute.name));
        }

        size_t accessorIndex = attributes[attribute.name].get<size_t>();
        try
        {
            if (attribute.name == "POSITION")
            {
                mesh.positions_ = LoadAttribute<glm::vec3>(
                    gltf, basePath, accessorIndex, attribute.type, attribute.componentType);
            }
            else if (attribute.name == "NORMAL")
            {
                mesh.normals_ = LoadAttribute<glm::vec3>(
                    gltf, basePath, accessorIndex, attribute.type, attribute.componentType);
            }
            else if (attribute.name == "TANGENT")
            {
                mesh.tangents_ = LoadAttribute<glm::vec4>(
                    gltf, basePath, accessorIndex, attribute.type, attribute.componentType);
            }
            else if (attribute.name == "TEXCOORD_0")
            {
                mesh.texCoord0_ = LoadAttribute<glm::vec2>(
                    gltf, basePath, accessorIndex, attribute.type, attribute.componentType);
            }
            else if (attribute.name == "TEXCOORD_1")
            {
                mesh.texCoord1_ = LoadAttribute<glm::vec2>(
                    gltf, basePath, accessorIndex, attribute.type, attribute.componentType);
            }
            else if (attribute.name == "COLOR_0")
            {
                mesh.color0_ = LoadAttribute<glm::vec4>(
                    gltf, basePath, accessorIndex, attribute.type, attribute.componentType);
            }
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(
                std::format("Failed to load {} attribute: {}", attribute.name, e.what()));
        }
    }

    return mesh;
}

glm::vec3 Mesh::GetAABBMin() const noexcept
{
    return std::accumulate(positions_.begin(), positions_.end(),
        glm::vec3(std::numeric_limits<float>::max()),
        [](const auto& a, const auto& b) { return glm::min(a, b); });
}

glm::vec3 Mesh::GetAABBMax() const noexcept
{
    return std::accumulate(positions_.begin(), positions_.end(),
        glm::vec3(std::numeric_limits<float>::lowest()),
        [](const auto& a, const auto& b) { return glm::max(a, b); });
}
