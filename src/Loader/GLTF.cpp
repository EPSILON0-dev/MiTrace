#include "GLTF.hpp"

#include <spdlog/spdlog.h>

#include <format>
#include <fstream>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>

GLTF::GLTF(const std::filesystem::path& filePath)
    : filePath_(filePath), basePath_(filePath.parent_path())
{
    try
    {
        gltfData_ =
            std::make_unique<nlohmann::json>(nlohmann::json::parse(std::ifstream(filePath_)));
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(
            std::format("Failed to load GLTF file '{}': {}", filePath_.string(), e.what()));
    }

    spdlog::info("Loaded GLTF file \"{}\"", filePath_.filename().string());
}

GLTF::~GLTF() = default;

const std::vector<uint8_t>& GLTF::GetBufferData(size_t bufferIndex)
{
    // If we found it in the cache, return it
    if (loadedBuffers_.find(bufferIndex) != loadedBuffers_.end())
        return loadedBuffers_.at(bufferIndex);

    // Check for data URI buffers (not supported)
    const auto& bufferData = (*gltfData_)["buffers"][bufferIndex];
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

GLTF::Accessor GLTF::ParseAccessor(size_t accessorIndex)
{
    const auto& accessorData = (*gltfData_)["accessors"][accessorIndex];

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

GLTF::BufferView GLTF::ParseBufferView(size_t bufferViewIndex)
{
    const auto& bufferViewData = (*gltfData_)["bufferViews"][bufferViewIndex];

    BufferView bufferView;
    bufferView.bufferIndex = bufferViewData["buffer"].get<size_t>();
    bufferView.byteOffset = bufferViewData.value("byteOffset", 0);
    bufferView.byteLength = bufferViewData["byteLength"].get<size_t>();
    bufferView.byteStride = bufferViewData.value("byteStride", 0);

    return bufferView;
}

template <typename T>
std::vector<T> GLTF::LoadMeshAttributeData(const BufferView& bufferView, const Accessor& accessor,
    AttributeType expectedType, ComponentType expectedComponentType)
{
    if (accessor.attributeType != expectedType || accessor.componentType != expectedComponentType)
        throw std::runtime_error("Attribute type does not match expected type");

    const auto& bufferData = GetBufferData(bufferView.bufferIndex);

    std::vector<T> data;
    data.reserve(accessor.count);

    // We assume tightly packed data with the same byte order as the host architecture
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

bool GLTF::IsAccessorCorrect(
    size_t accessorIndex, AttributeType expectedType, ComponentType expectedComponentType)
{
    Accessor accessor = ParseAccessor(accessorIndex);
    return accessor.attributeType == expectedType &&
           accessor.componentType == expectedComponentType;
}

template <typename T>
std::vector<T> GLTF::LoadMeshAttribute(
    size_t accessorIndex, AttributeType expectedType, ComponentType expectedComponentType)
{
    Accessor accessor = ParseAccessor(accessorIndex);
    BufferView bufferView = ParseBufferView(accessor.bufferViewIndex);

    return LoadMeshAttributeData<T>(bufferView, accessor, expectedType, expectedComponentType);
}

Texture GLTF::LoadTexture(size_t textureIndex)
{
    const auto& textureData = (*gltfData_)["textures"][textureIndex];
    size_t imageIndex = textureData["source"].get<size_t>();
    auto imagePtr = LoadImage(imageIndex);

    // Check the coord set
    if (textureData.contains("texCoord"))
    {
        size_t texCoordSet = textureData["texCoord"].get<size_t>();
        if (texCoordSet != 0) throw std::runtime_error("Only TEXCOORD_0 is supported");
    }

    TextureImage::FilterMode filterMode = TextureImage::FilterMode::Linear;
    const static std::map<size_t, TextureImage::FilterMode> filterModeMap = {
        {9728, TextureImage::FilterMode::Nearest},  // NEAREST
        {9984, TextureImage::FilterMode::Nearest},  // NEAREST
        {9985, TextureImage::FilterMode::Nearest},  // NEAREST
        {9729, TextureImage::FilterMode::Linear},   // LINEAR
        {9986, TextureImage::FilterMode::Linear},   // LINEAR
        {9987, TextureImage::FilterMode::Linear}};  // LINEAR

    if (textureData.contains("sampler"))
    {
        const auto& samplerData =
            (*gltfData_)["samplers"][textureData["sampler"].get<size_t>()].value("minFilter", 9729);
        try
        {
            filterMode = filterModeMap.at(samplerData);
        }
        catch (const std::out_of_range&)
        {
            throw std::runtime_error("Unsupported filter mode in texture sampler");
        }
    }

    return Texture(imagePtr, filterMode);
}

glm::mat4 GLTF::ComputeNodeTransform(size_t nodeIndex) const
{
    const auto& nodeData = (*gltfData_)["nodes"][nodeIndex];

    glm::mat4 transform = glm::mat4(1.0f);
    if (nodeData.contains("matrix"))
    {
        transform = glm::make_mat4(nodeData["matrix"].get<std::vector<float>>().data());
    }
    else
    {
        if (nodeData.contains("translation"))
        {
            glm::vec3 translation =
                glm::make_vec3(nodeData["translation"].get<std::vector<float>>().data());
            transform = glm::translate(transform, translation);
        }
        if (nodeData.contains("rotation"))
        {
            glm::quat rotation =
                glm::make_quat(nodeData["rotation"].get<std::vector<float>>().data());
            transform *= glm::mat4_cast(rotation);
        }
        if (nodeData.contains("scale"))
        {
            glm::vec3 scale = glm::make_vec3(nodeData["scale"].get<std::vector<float>>().data());
            transform = glm::scale(transform, scale);
        }
    }

    return transform;
}

std::shared_ptr<Mesh> GLTF::LoadMesh(size_t meshIndex, size_t primitiveIndex)
{
    // If we found it in the cache, return it
    if (loadedMeshes_.find(std::pair(meshIndex, primitiveIndex)) != loadedMeshes_.end())
        return loadedMeshes_.at(std::pair(meshIndex, primitiveIndex));

    // Create a new mesh builder
    auto meshPtr = std::make_shared<Mesh>();
    auto& mesh = *meshPtr;

    // Load the useless name
    mesh.SetName((*gltfData_)["meshes"][meshIndex].value("name", "Unnamed_Mesh"));

    // Validate primitive data
    const auto& primitiveData = (*gltfData_)["meshes"][meshIndex]["primitives"][primitiveIndex];
    if (!primitiveData.contains("attributes") || !primitiveData.contains("indices"))
        throw std::runtime_error("Invalid GLTF primitive data: missing attributes or indices");
    if (primitiveData.contains("mode") && primitiveData["mode"] != 4)
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
            mesh.SetIndices(LoadMeshAttribute<uint32_t>(
                indicesAcc, AttributeType::SCALAR, ComponentType::UNSIGNED_INT));
        }
        else
        {
            auto shortIndices = LoadMeshAttribute<uint16_t>(
                indicesAcc, AttributeType::SCALAR, ComponentType::UNSIGNED_SHORT);

            auto indices = std::vector<uint32_t>();
            indices.reserve(shortIndices.size());
            std::transform(shortIndices.begin(), shortIndices.end(), std::back_inserter(indices),
                [](uint16_t index) { return static_cast<uint32_t>(index); });
            mesh.SetIndices(std::move(indices));
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
                mesh.SetPositions(LoadMeshAttribute<glm::vec3>(
                    accessorIndex, attribute.type, attribute.componentType));
            }
            else if (attribute.name == "NORMAL")
            {
                mesh.SetNormals(LoadMeshAttribute<glm::vec3>(
                    accessorIndex, attribute.type, attribute.componentType));
            }
            else if (attribute.name == "TANGENT")
            {
                mesh.SetTangents(LoadMeshAttribute<glm::vec4>(
                    accessorIndex, attribute.type, attribute.componentType));
            }
            else if (attribute.name == "TEXCOORD_0")
            {
                mesh.SetTexCoord0(LoadMeshAttribute<glm::vec2>(
                    accessorIndex, attribute.type, attribute.componentType));
            }
            else if (attribute.name == "TEXCOORD_1")
            {
                mesh.SetTexCoord1(LoadMeshAttribute<glm::vec2>(
                    accessorIndex, attribute.type, attribute.componentType));
            }
            else if (attribute.name == "COLOR_0")
            {
                mesh.SetColor0(LoadMeshAttribute<glm::vec4>(
                    accessorIndex, attribute.type, attribute.componentType));
            }
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(
                std::format("Failed to load {} attribute: {}", attribute.name, e.what()));
        }
    }

    // Build the mesh
    spdlog::trace(
        "Loaded new mesh \"{}\", triangles: {}", meshPtr->GetName(), meshPtr->GetTriangleCount());

    // Cache and return the loaded mesh
    loadedMeshes_.emplace(std::pair(meshIndex, primitiveIndex), meshPtr);
    return meshPtr;
}

Material GLTF::LoadMeshMaterial(size_t meshIndex, size_t primitiveIndex)
{
    const auto& primitiveData = (*gltfData_)["meshes"][meshIndex]["primitives"][primitiveIndex];

    // If no material is specified, return a default material
    if (!primitiveData.contains("material")) return Material();

    size_t materialIndex = primitiveData["material"].get<size_t>();
    return LoadMaterial(materialIndex);
}

std::shared_ptr<TextureImage> GLTF::LoadImage(size_t imageIndex)
{
    // Return from cache if already loaded
    if (loadedImages_.find(imageIndex) != loadedImages_.end()) return loadedImages_.at(imageIndex);

    // Load the image
    auto& imageData = (*gltfData_)["images"][imageIndex];
    if (!imageData.contains("uri")) throw std::runtime_error("Only URI-based images are supported");
    auto image = std::make_shared<TextureImage>(basePath_ / imageData["uri"].get<std::string>());
    image->SetName(imageData.value("name", "Unnamed_Image"));

    spdlog::trace("Loaded new image \"{}\" ({}x{}, {} channels)", image->GetName(),
        image->GetWidth(), image->GetHeight(), image->GetChannels());

    // Emplace in cache and return
    loadedImages_.emplace(imageIndex, image);
    return image;
}

Material GLTF::LoadMaterial(size_t materialIndex)
{
    const auto& materialData = (*gltfData_)["materials"][materialIndex];
    const auto& pbrData = materialData["pbrMetallicRoughness"];
    Material material;

    // Load emissive texture and factor
    if (pbrData.contains("emissiveTexture"))
        material.SetEmissiveTexture(LoadTexture(pbrData["emissiveTexture"]["index"].get<size_t>()));
    material.SetEmissiveFactor(
        pbrData.contains("emissiveFactor")
            ? glm::make_vec3(pbrData["emissiveFactor"].get<std::vector<float>>().data())
            : glm::vec3(1.0f));

    // Load PBR material properties
    material.SetName(materialData.value("name", "Unnamed_Material"));
    material.SetBaseColorFactor(
        pbrData.contains("baseColorFactor")
            ? glm::make_vec4(pbrData["baseColorFactor"].get<std::vector<float>>().data())
            : glm::vec4(1.0f));
    material.SetMetallicFactor(pbrData.value("metallicFactor", 1.0f));
    material.SetRoughnessFactor(pbrData.value("roughnessFactor", 1.0f));

    // Load textures if present
    if (pbrData.contains("baseColorTexture"))
        material.SetBaseColorTexture(
            LoadTexture(pbrData["baseColorTexture"]["index"].get<size_t>()));
    if (pbrData.contains("metallicRoughnessTexture"))
        material.SetMetallicRoughnessTexture(
            LoadTexture(pbrData["metallicRoughnessTexture"]["index"].get<size_t>()));

    // Load normal texture if present
    if (materialData.contains("normalTexture"))
        material.SetNormalTexture(
            LoadTexture(materialData["normalTexture"]["index"].get<size_t>()));
    material.SetNormalScale(materialData.value("normalTexture", 1.0f));

    // Load occlusion texture if present
    if (materialData.contains("occlusionTexture"))
        material.SetOcclusionTexture(
            LoadTexture(materialData["occlusionTexture"]["index"].get<size_t>()));
    material.SetOcclusionStrength(materialData.value("occlusionStrength", 1.0f));

    // Load alpha properties
    static const std::map<std::string, Material::TransparencyMode> alphaModeMap = {
        {"OPAQUE", Material::TransparencyMode::OPAQUE},  // OPAQUE
        {"MASK", Material::TransparencyMode::MASK},      // MASK
        {"BLEND", Material::TransparencyMode::BLEND}};   // BLEND
    std::string alphaMode = materialData.value("alphaMode", "OPAQUE");
    try
    {
        material.SetTransparencyMode(alphaModeMap.at(alphaMode));
    }
    catch (const std::out_of_range&)
    {
        throw std::runtime_error("Unsupported alpha mode in material");
    }
    material.SetAlphaCutoff(materialData.value("alphaCutoff", 0.5f));
    material.SetDoubleSided(materialData.value("doubleSided", false));

    spdlog::trace("Loaded new material \"{}\"", material.GetName());
    return material;
}

Camera GLTF::LoadCamera(size_t cameraIndex) const
{
    const auto& cameraData = (*gltfData_)["cameras"][cameraIndex];

    std::string type = cameraData["type"].get<std::string>();
    if (type != "perspective")
        throw std::runtime_error(std::format("Unsupported camera type in GLTF: {}", type));

    const auto& perspectiveData = cameraData["perspective"];
    float yfov = perspectiveData["yfov"].get<float>();

    // We don't need these parameters
    // float aspectRatio = perspectiveData.value("aspectRatio", 1.0f);
    // float znear = perspectiveData["znear"].get<float>();
    // float zfar = perspectiveData.value("zfar", 1000.0f);

    return Camera(yfov);
}

Light::PointLight GLTF::LoadPointLight(size_t lightIndex) const
{
    const auto& lightData = (*gltfData_)["extensions"]["KHR_lights_punctual"]["lights"][lightIndex];

    Light::PointLight pointLight;
    if (lightData.contains("color"))
        pointLight.Color = glm::make_vec3(lightData["color"].get<std::vector<float>>().data());
    else
        pointLight.Color = glm::vec3(1.0f);
    pointLight.Intensity = lightData.value("intensity", 1.0f);
    pointLight.Range = lightData.value("range", 0.0f);
    pointLight.Size = lightData.value("size", 0.0f);
    return pointLight;
}

Light::DirectionalLight GLTF::LoadDirectionalLight(size_t lightIndex) const
{
    const auto& lightData = (*gltfData_)["extensions"]["KHR_lights_punctual"]["lights"][lightIndex];

    Light::DirectionalLight directionalLight;
    if (lightData.contains("color"))
        directionalLight.Color =
            glm::make_vec3(lightData["color"].get<std::vector<float>>().data());
    else
        directionalLight.Color = glm::vec3(1.0f);
    directionalLight.Intensity = lightData.value("intensity", 1.0f);
    directionalLight.Angle = lightData.value("angle", 0.0f);
    return directionalLight;
}

Light::SpotLight GLTF::LoadSpotLight(size_t lightIndex) const
{
    const auto& lightData = (*gltfData_)["extensions"]["KHR_lights_punctual"]["lights"][lightIndex];

    Light::SpotLight spotLight;
    if (lightData.contains("color"))
        spotLight.Color = glm::make_vec3(lightData["color"].get<std::vector<float>>().data());
    else
        spotLight.Color = glm::vec3(1.0f);
    spotLight.Intensity = lightData.value("intensity", 1.0f);
    spotLight.Range = lightData.value("range", 0.0f);
    spotLight.Size = lightData.value("size", 0.0f);
    spotLight.InnerConeAngle = lightData["spot"].value("innerConeAngle", 0.0f);
    spotLight.OuterConeAngle = lightData["spot"].value("outerConeAngle", glm::quarter_pi<float>());
    return spotLight;
}

Light::AreaLight GLTF::LoadAreaLight(size_t lightIndex) const
{
    const auto& lightData = (*gltfData_)["extensions"]["KHR_lights_punctual"]["lights"][lightIndex];

    if (!lightData.contains("area_size"))
        throw std::runtime_error("Area light missing 'area_size' property");

    Light::AreaLight areaLight;
    if (lightData.contains("color"))
        areaLight.Color = glm::make_vec3(lightData["color"].get<std::vector<float>>().data());
    else
        areaLight.Color = glm::vec3(1.0f);
    areaLight.Intensity = lightData.value("intensity", 1.0f);
    areaLight.Range = lightData.value("range", 0.0f);
    areaLight.Size = glm::make_vec2(lightData["area_size"].get<std::vector<float>>().data());
    return areaLight;
}

Light GLTF::LoadLight(size_t lightIndex, const glm::mat4& transform)
{
    const auto& lightData = (*gltfData_)["extensions"]["KHR_lights_punctual"]["lights"][lightIndex];

    static const std::map<std::string, Light::Type> lightTypeMap = {
        {"point", Light::Type::POINT},              // POINT
        {"directional", Light::Type::DIRECTIONAL},  // DIRECTIONAL
        {"spot", Light::Type::SPOT},                // SPOT
        {"area", Light::Type::AREA}};               // AREA
    Light::Type type;
    try
    {
        type = lightTypeMap.at(lightData["type"].get<std::string>());
    }
    catch (const std::out_of_range&)
    {
        throw std::runtime_error("Unsupported light type in GLTF");
    }

    switch (type)
    {
        case Light::Type::POINT:
            return Light(transform, LoadPointLight(lightIndex));
        case Light::Type::DIRECTIONAL:
            return Light(transform, LoadDirectionalLight(lightIndex));
        case Light::Type::SPOT:
            return Light(transform, LoadSpotLight(lightIndex));
        case Light::Type ::AREA:
            return Light(transform, LoadAreaLight(lightIndex));
        default:
            throw std::runtime_error("Unsupported light type in GLTF");
    }
}

std::vector<MeshInstance> GLTF::LoadNodeMeshes(size_t nodeIndex, const glm::mat4& transform)
{
    std::vector<MeshInstance> instances;
    const auto& nodeData = (*gltfData_)["nodes"][nodeIndex];

    glm::mat4 worldTransform = transform * ComputeNodeTransform(nodeIndex);

    if (nodeData.contains("mesh"))
    {
        try
        {
            const auto primitiveCount =
                (*gltfData_)["meshes"][nodeData["mesh"].get<size_t>()]["primitives"].size();
            for (size_t primitiveIndex = 0; primitiveIndex < primitiveCount; ++primitiveIndex)
            {
                auto meshIndex = nodeData["mesh"].get<size_t>();
                auto meshPtr = LoadMesh(meshIndex, primitiveIndex);
                auto material = LoadMeshMaterial(meshIndex, primitiveIndex);
                instances.push_back(MeshInstance(std::move(meshPtr), material, worldTransform));
            }
        }
        catch (const std::exception& e)
        {
            const auto error =
                std::format("Failed to load mesh on node {}: {}", nodeIndex, e.what());
            spdlog::error(error);
            throw std::runtime_error(error);
        }
    }

    if (nodeData.contains("children"))
    {
        const auto& children = nodeData["children"];
        for (const auto& childIndex : children)
        {
            auto childInstances = LoadNodeMeshes(childIndex.get<size_t>(), worldTransform);
            instances.insert(instances.end(), std::make_move_iterator(childInstances.begin()),
                std::make_move_iterator(childInstances.end()));
        }
    }

    return instances;
}

std::vector<MeshInstance> GLTF::LoadSceneMeshes(size_t sceneIndex, const glm::mat4& transform)
{
    const auto& sceneData = (*gltfData_)["scenes"][sceneIndex];

    std::vector<MeshInstance> instances;
    for (const auto& nodeIndex : sceneData["nodes"])
    {
        auto nodeInstances = LoadNodeMeshes(nodeIndex.get<size_t>(), transform);
        instances.insert(instances.end(), std::make_move_iterator(nodeInstances.begin()),
            std::make_move_iterator(nodeInstances.end()));
    }

    return instances;
}

std::vector<Light> GLTF::LoadNodeLights(size_t nodeIndex, const glm::mat4& transform)
{
    std::vector<Light> lights;
    const auto& nodeData = (*gltfData_)["nodes"][nodeIndex];

    glm::mat4 worldTransform = transform * ComputeNodeTransform(nodeIndex);

    if (nodeData.contains("extensions") && nodeData["extensions"].contains("KHR_lights_punctual"))
    {
        try
        {
            auto lightIndex = nodeData["extensions"]["KHR_lights_punctual"]["light"].get<size_t>();
            lights.push_back(LoadLight(lightIndex, worldTransform));

            spdlog::trace("Loaded light {} on node {} ({})", lightIndex,
                nodeData.value("name", "Unnamed_Node"), nodeIndex);
        }
        catch (const std::exception& e)
        {
            const auto error =
                std::format("Failed to load light on node {}: {}", nodeIndex, e.what());
            spdlog::error(error);
            throw std::runtime_error(error);
        }
    }

    if (nodeData.contains("children"))
    {
        const auto& children = nodeData["children"];
        for (const auto& childIndex : children)
        {
            auto childInstances = LoadNodeLights(childIndex.get<size_t>(), worldTransform);
            lights.insert(lights.end(), std::make_move_iterator(childInstances.begin()),
                std::make_move_iterator(childInstances.end()));
        }
    }

    return lights;
}

std::vector<Light> GLTF::LoadSceneLights(size_t sceneIndex, const glm::mat4& transform)
{
    const auto& sceneData = (*gltfData_)["scenes"][sceneIndex];

    std::vector<Light> lights;
    for (const auto& nodeIndex : sceneData["nodes"])
    {
        auto nodeInstances = LoadNodeLights(nodeIndex.get<size_t>(), transform);
        lights.insert(lights.end(), std::make_move_iterator(nodeInstances.begin()),
            std::make_move_iterator(nodeInstances.end()));
    }

    return lights;
}

std::vector<Camera> GLTF::LoadNodeCameras(size_t nodeIndex, const glm::mat4& transform) const
{
    std::vector<Camera> cameras;
    const auto& nodeData = (*gltfData_)["nodes"][nodeIndex];
    glm::mat4 worldTransform = transform * ComputeNodeTransform(nodeIndex);

    if (nodeData.contains("camera"))
    {
        try
        {
            size_t cameraIndex = nodeData["camera"].get<size_t>();
            Camera camera = LoadCamera(cameraIndex);
            camera.SetCameraToWorld(worldTransform);
            cameras.push_back(camera);

            spdlog::trace("Loaded camera {} on node {} ({})", cameraIndex,
                nodeData.value("name", "Unnamed_Node"), nodeIndex);
        }
        catch (const std::exception& e)
        {
            const auto error =
                std::format("Failed to load camera on node {}: {}", nodeIndex, e.what());
            spdlog::error(error);
            throw std::runtime_error(error);
        }
    }

    if (nodeData.contains("children"))
    {
        const auto& children = nodeData["children"];
        for (const auto& childIndex : children)
        {
            auto childCameras = LoadNodeCameras(childIndex.get<size_t>(), worldTransform);
            cameras.insert(cameras.end(), std::make_move_iterator(childCameras.begin()),
                std::make_move_iterator(childCameras.end()));
        }
    }

    return cameras;
}

std::vector<Camera> GLTF::LoadSceneCameras(size_t sceneIndex, const glm::mat4& transform) const
{
    std::vector<Camera> cameras;
    const auto& sceneData = (*gltfData_)["scenes"][sceneIndex];

    for (const auto& nodeIndex : sceneData["nodes"])
    {
        auto nodeCameras = LoadNodeCameras(nodeIndex.get<size_t>(), transform);
        cameras.insert(cameras.end(), std::make_move_iterator(nodeCameras.begin()),
            std::make_move_iterator(nodeCameras.end()));
    }

    return cameras;
}

Camera GLTF::LoadSceneCamera(size_t sceneIndex, const glm::mat4& transform) const
{
    const auto cameras = LoadSceneCameras(sceneIndex, transform);

    if (cameras.empty()) throw std::runtime_error("No camera found in the scene");

    if (cameras.size() > 1)
        spdlog::warn("Multiple cameras found in the scene, using the first one loaded");

    return cameras.front();
}

template <typename Tmap, typename Tlambda>
static auto CleanupMap(Tmap& map, Tlambda check)
{
    Tmap cleanedMap;
    std::copy_if(map.begin(), map.end(), std::inserter(cleanedMap, cleanedMap.end()), check);
    map = std::move(cleanedMap);
}

std::optional<Texture> GLTF::LoadSceneEnvironmentTexture()
{
    if ((*gltfData_).contains("extensions") && (*gltfData_)["extensions"].contains("EXT_sky") &&
        (*gltfData_)["extensions"]["EXT_sky"].contains("sky_texture"))
    {
        const auto textureIndex =
            (*gltfData_)["extensions"]["EXT_sky"]["sky_texture"].get<size_t>();
        return LoadTexture(textureIndex);
    }

    return std::nullopt;
}

Scene GLTF::LoadScene(size_t sceneIndex, const glm::mat4& transform)
{
    Scene scene;

    // Load meshes
    auto meshInstances = LoadSceneMeshes(sceneIndex, transform);
    for (auto& instance : meshInstances) scene.AddMeshInstance(std::move(instance));

    // Load lights
    auto lights = LoadSceneLights(sceneIndex, transform);
    for (auto& light : lights) scene.AddLight(std::move(light));

    // Load environment texture if present
    auto envTexture = LoadSceneEnvironmentTexture();
    if (envTexture.has_value())
    {
        scene.SetEnvironmentTexture(envTexture.value());
        spdlog::info("Loaded environment texture for the scene");
    }

    spdlog::info("Loaded scene {} with {} mesh instances and {} lights", sceneIndex,
        scene.GetMeshInstances().size(), scene.GetLights().size());

    return scene;
}

void GLTF::Cleanup()
{
    size_t droppedBuffers = loadedBuffers_.size();
    [[maybe_unused]] size_t initialMaterialCount = loadedMaterials_.size();
    [[maybe_unused]] size_t initialImagesCount = loadedImages_.size();
    [[maybe_unused]] size_t initialMeshCount = loadedMeshes_.size();

    loadedBuffers_.clear();

    // Drop materials, images and meshes that only have one reference (the one in the loader)
    auto usedCheck = [](const auto& pair) { return (pair.second.use_count() > 1); };
    CleanupMap(loadedMaterials_, usedCheck);
    CleanupMap(loadedImages_, usedCheck);
    CleanupMap(loadedMeshes_, usedCheck);

    spdlog::debug(
        "GLTF Loader cleaned up unused resources, dropped {} buffers, {} materials, {} images, and "
        "{} meshes",
        droppedBuffers, initialMaterialCount - loadedMaterials_.size(),
        initialImagesCount - loadedImages_.size(), initialMeshCount - loadedMeshes_.size());
}
