#include "GLTF_Loader.hpp"

#include <format>
#include <fstream>
#include <stdexcept>
#include <memory>

GLTF_Loader::GLTF_Loader(const std::filesystem::path& filePath)
    : filePath_(filePath), basePath_(filePath.parent_path())
{
    try
    {
        gltfData_ = nlohmann::json::parse(std::ifstream(filePath_));
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(
            std::format("Failed to load GLTF file '{}': {}", filePath_.string(), e.what()));
    }
}

const std::vector<uint8_t>& GLTF_Loader::GetBufferData(size_t bufferIndex)
{
    // If we found it in the cache, return it
    if (loadedBuffers_.find(bufferIndex) != loadedBuffers_.end())
        return loadedBuffers_.at(bufferIndex);

    // Check for data URI buffers (not supported)
    const auto& bufferData = gltfData_["buffers"][bufferIndex];
    if (bufferData["uri"].get<std::string>().rfind("data:", 0) == 0)
        throw std::runtime_error("Data URI buffers are not supported");

    // Open the buffer file
    std::filesystem::path bufferPath = basePath_ / bufferData["uri"].get<std::string>();
    std::ifstream bufferFile(bufferPath, std::ios::binary);
    if (!bufferFile)
        throw std::runtime_error(
            std::format("Failed to open buffer file: {}", bufferPath.string()));

    // Read the buffer data
    size_t byteLength = bufferData["byteLength"].get<size_t>();
    std::vector<uint8_t> bufferBytes(byteLength);
    bufferFile.read(reinterpret_cast<char*>(bufferBytes.data()), byteLength);
    if (!bufferFile)
        throw std::runtime_error(
            std::format("Failed to read buffer file: {}", bufferPath.string()));

    // Cache and return the loaded buffer data
    loadedBuffers_.emplace(bufferIndex, std::move(bufferBytes));
    return loadedBuffers_.at(bufferIndex);
}

GLTF_Loader::Accessor GLTF_Loader::ParseAccessor(size_t accessorIndex)
{
    const auto& accessorData = gltfData_["accessors"][accessorIndex];

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

GLTF_Loader::BufferView GLTF_Loader::ParseBufferView(size_t bufferViewIndex)
{
    const auto& bufferViewData = gltfData_["bufferViews"][bufferViewIndex];

    BufferView bufferView;
    bufferView.bufferIndex = bufferViewData["buffer"].get<size_t>();
    bufferView.byteOffset = bufferViewData.value("byteOffset", 0);
    bufferView.byteLength = bufferViewData["byteLength"].get<size_t>();
    bufferView.byteStride = bufferViewData.value("byteStride", 0);

    return bufferView;
}

template <typename T>
std::vector<T> GLTF_Loader::LoadMeshAttributeData(const BufferView& bufferView,
    const Accessor& accessor, AttributeType expectedType, ComponentType expectedComponentType)
{
    if (accessor.attributeType != expectedType || accessor.componentType != expectedComponentType)
        throw std::runtime_error("Attribute type does not match expected type");

    const auto& bufferData = GetBufferData(bufferView.bufferIndex);

    std::vector<T> data;
    data.reserve(accessor.count);

    // We assume tightly packed data with the same byte order as the host architecture
    // TODO: Maybe there's a better way to handle this?
    size_t stride = accessor.attributeCount * accessor.componentSize;
    for (size_t i = 0; i < accessor.count; ++i)
    {
        size_t offset = accessor.byteOffset + i * stride + bufferView.byteOffset;
        T value;
        std::memcpy(&value, bufferData.data() + offset, sizeof(T));
        data.push_back(value);
    }

    return data;
}

bool GLTF_Loader::IsAccessorCorrect(
    size_t accessorIndex, AttributeType expectedType, ComponentType expectedComponentType)
{
    Accessor accessor = ParseAccessor(accessorIndex);
    return accessor.attributeType == expectedType &&
           accessor.componentType == expectedComponentType;
}

template <typename T>
std::vector<T> GLTF_Loader::LoadMeshAttribute(
    size_t accessorIndex, AttributeType expectedType, ComponentType expectedComponentType)
{
    Accessor accessor = ParseAccessor(accessorIndex);
    BufferView bufferView = ParseBufferView(accessor.bufferViewIndex);

    return LoadMeshAttributeData<T>(bufferView, accessor, expectedType, expectedComponentType);
}

std::shared_ptr<Mesh> GLTF_Loader::LoadMesh(size_t meshIndex, size_t primitiveIndex)
{
    // If we found it in the cache, return it
    if (loadedMeshes_.find(meshIndex) != loadedMeshes_.end())
        return loadedMeshes_.at(meshIndex);

    // Create a new mesh
    auto meshPtr = std::make_shared<Mesh>();
    auto& mesh = *meshPtr;

    // Validate primitive data
    const auto& primitiveData = gltfData_["meshes"][meshIndex]["primitives"][primitiveIndex];
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
            IsAccessorCorrect(indicesAcc, AttributeType::SCALAR, ComponentType::UNSIGNED_INT);
        bool areIndicesUnsignedShort =
            IsAccessorCorrect(indicesAcc, AttributeType::SCALAR, ComponentType::UNSIGNED_SHORT);

        if (!areIndicesUnsignedInt && !areIndicesUnsignedShort)
            throw std::runtime_error("Indices accessor has unsupported component type");

        if (areIndicesUnsignedInt)
        {
            mesh.indices_ = LoadMeshAttribute<uint32_t>(
                indicesAcc, AttributeType::SCALAR, ComponentType::UNSIGNED_INT);
        }
        else
        {
            auto shortIndices = LoadMeshAttribute<uint16_t>(
                indicesAcc, AttributeType::SCALAR, ComponentType::UNSIGNED_SHORT);
            mesh.indices_.reserve(shortIndices.size());
            std::transform(shortIndices.begin(), shortIndices.end(),
                std::back_inserter(mesh.indices_),
                [](uint16_t index) { return static_cast<uint32_t>(index); });
        }
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::format("Failed to load INDICES: {}", e.what()));
    }

    // Load the rest of the attributes
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
                mesh.positions_ = LoadMeshAttribute<glm::vec3>(
                    accessorIndex, attribute.type, attribute.componentType);
            }
            else if (attribute.name == "NORMAL")
            {
                mesh.normals_ = LoadMeshAttribute<glm::vec3>(
                    accessorIndex, attribute.type, attribute.componentType);
            }
            else if (attribute.name == "TANGENT")
            {
                mesh.tangents_ = LoadMeshAttribute<glm::vec4>(
                    accessorIndex, attribute.type, attribute.componentType);
            }
            else if (attribute.name == "TEXCOORD_0")
            {
                mesh.texCoord0_ = LoadMeshAttribute<glm::vec2>(
                    accessorIndex, attribute.type, attribute.componentType);
            }
            else if (attribute.name == "TEXCOORD_1")
            {
                mesh.texCoord1_ = LoadMeshAttribute<glm::vec2>(
                    accessorIndex, attribute.type, attribute.componentType);
            }
            else if (attribute.name == "COLOR_0")
            {
                mesh.color0_ = LoadMeshAttribute<glm::vec4>(
                    accessorIndex, attribute.type, attribute.componentType);
            }
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(
                std::format("Failed to load {} attribute: {}", attribute.name, e.what()));
        }
    }

    // Cache and return the loaded mesh
    loadedMeshes_[meshIndex] = meshPtr;
    return meshPtr;
}
