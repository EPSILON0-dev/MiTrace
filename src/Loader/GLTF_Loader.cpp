#include "GLTF_Loader.hpp"

#include <format>
#include <fstream>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <stdexcept>

#include "Common/Logger.pch.hpp"  // IWYU pragma: keep
#include "Trace/MaterialGLTF.hpp"

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

    SPDLOG_INFO("Loaded GLTF file \"{}\"", filePath_.filename().string());
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

Texture GLTF_Loader::LoadTexture(size_t textureIndex)
{
    const auto& textureData = gltfData_["textures"][textureIndex];
    size_t imageIndex = textureData["source"].get<size_t>();
    auto imagePtr = LoadImage(imageIndex);

    // Check the coord set
    if (textureData.contains("texCoord"))
    {
        size_t texCoordSet = textureData["texCoord"].get<size_t>();
        if (texCoordSet != 0) throw std::runtime_error("Only TEXCOORD_0 is supported");
    }

    Image::FilterMode filterMode = Image::FilterMode::Linear;
    const static std::map<size_t, Image::FilterMode> filterModeMap = {
        {9728, Image::FilterMode::Nearest},  // NEAREST
        {9729, Image::FilterMode::Linear}};  // LINEAR

    if (textureData.contains("sampler"))
    {
        const auto& samplerData =
            gltfData_["samplers"][textureData["sampler"].get<size_t>()].value("minFilter", 9729);
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

glm::mat4 GLTF_Loader::ComputeNodeTransform(size_t nodeIndex) const
{
    const auto& nodeData = gltfData_["nodes"][nodeIndex];

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

std::shared_ptr<Mesh> GLTF_Loader::LoadMesh(size_t meshIndex, size_t primitiveIndex)
{
    // If we found it in the cache, return it
    if (loadedMeshes_.find(std::pair(meshIndex, primitiveIndex)) != loadedMeshes_.end())
        return loadedMeshes_.at(std::pair(meshIndex, primitiveIndex));

    // Create a new mesh
    auto meshPtr = std::make_shared<Mesh>();
    auto& mesh = *meshPtr;

    // Load the useless name
    mesh.name_ = gltfData_["meshes"][meshIndex].value("name", "Unnamed_Mesh");

    // Validate primitive data
    const auto& primitiveData = gltfData_["meshes"][meshIndex]["primitives"][primitiveIndex];
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

    // Load the material if present
    if (primitiveData.contains("material"))
    {
        size_t materialIndex = primitiveData["material"].get<size_t>();
        auto materialPtr = LoadMaterial(materialIndex);
        mesh.SetMaterial(std::dynamic_pointer_cast<MaterialBase>(materialPtr));
    }

    SPDLOG_DEBUG("Loaded new mesh \"{}\", triangles: {}, attributes: P-N{}{}{}{}-I", mesh.name_,
        mesh.indices_.size() / 3, (mesh.tangents_.empty() ? "" : "-T"),
        (mesh.texCoord0_.empty() ? "" : "-U0"), (mesh.texCoord1_.empty() ? "" : "-U1"),
        (mesh.color0_.empty() ? "" : "-C0"));

    // Cache and return the loaded mesh
    loadedMeshes_.emplace(std::pair(meshIndex, primitiveIndex), meshPtr);
    return meshPtr;
}

std::shared_ptr<Image> GLTF_Loader::LoadImage(size_t imageIndex)
{
    // Return from cache if already loaded
    if (loadedImages_.find(imageIndex) != loadedImages_.end()) return loadedImages_.at(imageIndex);

    // Load the image
    auto& imageData = gltfData_["images"][imageIndex];
    if (!imageData.contains("uri")) throw std::runtime_error("Only URI-based images are supported");
    auto image = std::make_shared<Image>(basePath_ / imageData["uri"].get<std::string>());
    image->name_ = imageData.value("name", "Unnamed_Image");

    SPDLOG_DEBUG("Loaded new image \"{}\" ({}x{}, {} channels)", image->name_, image->width_,
        image->height_, image->channels_);

    // Emplace in cache and return
    loadedImages_.emplace(imageIndex, image);
    return image;
}

std::shared_ptr<MaterialGLTF> GLTF_Loader::LoadMaterial(size_t materialIndex)
{
    const auto& materialData = gltfData_["materials"][materialIndex];
    const auto& pbrData = materialData["pbrMetallicRoughness"];
    MaterialGLTF material;

    // Load emissive texture and factor
    if (pbrData.contains("emissiveTexture"))
        material.emissiveTexture_ = LoadTexture(pbrData["emissiveTexture"]["index"].get<size_t>());
    material.emissiveFactor_ =
        pbrData.contains("emissiveFactor")
            ? glm::make_vec3(pbrData["emissiveFactor"].get<std::vector<float>>().data())
            : glm::vec3(1.0f);

    // Load PBR material properties
    material.name_ = materialData.value("name", "Unnamed_Material");
    material.baseColorFactor_ =
        pbrData.contains("baseColorFactor")
            ? glm::make_vec4(pbrData["baseColorFactor"].get<std::vector<float>>().data())
            : glm::vec4(1.0f);
    material.metallicFactor_ = pbrData.value("metallicFactor", 1.0f);
    material.roughnessFactor_ = pbrData.value("roughnessFactor", 1.0f);

    // Load textures if present
    if (pbrData.contains("baseColorTexture"))
        material.baseColorTexture_ =
            LoadTexture(pbrData["baseColorTexture"]["index"].get<size_t>());
    if (pbrData.contains("metallicRoughnessTexture"))
        material.metallicRoughnessTexture_ =
            LoadTexture(pbrData["metallicRoughnessTexture"]["index"].get<size_t>());

    // Load normal texture if present
    if (materialData.contains("normalTexture"))
        material.normalTexture_ = LoadTexture(materialData["normalTexture"]["index"].get<size_t>());
    material.normalScale_ = materialData.value("normalTexture", 1.0f);

    // Load occlusion texture if present
    if (materialData.contains("occlusionTexture"))
        material.occlusionTexture_ =
            LoadTexture(materialData["occlusionTexture"]["index"].get<size_t>());
    material.occlusionStrength_ = materialData.value("occlusionStrength", 1.0f);

    // Load alpha properties
    static const std::map<std::string, MaterialGLTF::TransparencyMode> alphaModeMap = {
        {"OPAQUE", MaterialGLTF::TransparencyMode::OPAQUE},  // OPAQUE
        {"MASK", MaterialGLTF::TransparencyMode::MASK},      // MASK
        {"BLEND", MaterialGLTF::TransparencyMode::BLEND}};   // BLEND
    std::string alphaMode = materialData.value("alphaMode", "OPAQUE");
    try
    {
        material.transparencyMode_ = alphaModeMap.at(alphaMode);
    }
    catch (const std::out_of_range&)
    {
        throw std::runtime_error("Unsupported alpha mode in material");
    }
    material.alphaCutoff_ = materialData.value("alphaCutoff", 0.5f);
    material.doubleSided_ = materialData.value("doubleSided", false);

    SPDLOG_DEBUG("Loaded new material \"{}\"", material.name_);

    // Cache and return
    loadedMaterials_.emplace(materialIndex, std::make_shared<MaterialGLTF>(material));
    return loadedMaterials_.at(materialIndex);
}

Camera GLTF_Loader::LoadCamera(size_t cameraIndex) const
{
    const auto& cameraData = gltfData_["cameras"][cameraIndex];

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

Light::PointLight GLTF_Loader::LoadPointLight(size_t lightIndex) const
{
    const auto& lightData = gltfData_["extensions"]["KHR_lights_punctual"]["lights"][lightIndex];

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

Light::DirectionalLight GLTF_Loader::LoadDirectionalLight(size_t lightIndex) const
{
    const auto& lightData = gltfData_["extensions"]["KHR_lights_punctual"]["lights"][lightIndex];

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

Light::SpotLight GLTF_Loader::LoadSpotLight(size_t lightIndex) const
{
    const auto& lightData = gltfData_["extensions"]["KHR_lights_punctual"]["lights"][lightIndex];

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

Light::AreaLight GLTF_Loader::LoadAreaLight(size_t lightIndex) const
{
    const auto& lightData = gltfData_["extensions"]["KHR_lights_punctual"]["lights"][lightIndex];

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

Light GLTF_Loader::LoadLight(size_t lightIndex, const glm::mat4& transform)
{
    const auto& lightData = gltfData_["extensions"]["KHR_lights_punctual"]["lights"][lightIndex];

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

std::vector<MeshInstance> GLTF_Loader::LoadNodeMeshes(size_t nodeIndex, const glm::mat4& transform)
{
    std::vector<MeshInstance> instances;
    const auto& nodeData = gltfData_["nodes"][nodeIndex];

    glm::mat4 worldTransform = transform * ComputeNodeTransform(nodeIndex);

    if (nodeData.contains("mesh"))
    {
        try
        {
            const auto primitiveCount =
                gltfData_["meshes"][nodeData["mesh"].get<size_t>()]["primitives"].size();
            for (size_t primitiveIndex = 0; primitiveIndex < primitiveCount; ++primitiveIndex)
            {
                auto meshIndex = nodeData["mesh"].get<size_t>();
                auto meshPtr = LoadMesh(meshIndex, primitiveIndex);
                instances.push_back(MeshInstance(std::move(meshPtr), worldTransform));

                SPDLOG_DEBUG("Loaded mesh {} ({}:{}) on node {} ({})",
                    instances.back().GetMesh().GetName(), meshIndex, primitiveIndex,
                    nodeData.value("name", "Unnamed_Node"), nodeIndex);
            }
        }
        catch (const std::exception& e)
        {
            const auto error =
                std::format("Failed to load mesh on node {}: {}", nodeIndex, e.what());
            SPDLOG_ERROR(error);
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

std::vector<MeshInstance> GLTF_Loader::LoadSceneMeshes(
    size_t sceneIndex, const glm::mat4& transform)
{
    const auto& sceneData = gltfData_["scenes"][sceneIndex];

    std::vector<MeshInstance> instances;
    for (const auto& nodeIndex : sceneData["nodes"])
    {
        auto nodeInstances = LoadNodeMeshes(nodeIndex.get<size_t>(), transform);
        instances.insert(instances.end(), std::make_move_iterator(nodeInstances.begin()),
            std::make_move_iterator(nodeInstances.end()));
    }

    return instances;
}

std::vector<Light> GLTF_Loader::LoadNodeLights(size_t nodeIndex, const glm::mat4& transform)
{
    std::vector<Light> lights;
    const auto& nodeData = gltfData_["nodes"][nodeIndex];

    glm::mat4 worldTransform = transform * ComputeNodeTransform(nodeIndex);

    if (nodeData.contains("extensions") && nodeData["extensions"].contains("KHR_lights_punctual"))
    {
        try
        {
            auto lightIndex = nodeData["extensions"]["KHR_lights_punctual"]["light"].get<size_t>();
            lights.push_back(LoadLight(lightIndex, worldTransform));

            SPDLOG_DEBUG("Loaded light {} on node {} ({})", lightIndex,
                nodeData.value("name", "Unnamed_Node"), nodeIndex);
        }
        catch (const std::exception& e)
        {
            const auto error =
                std::format("Failed to load light on node {}: {}", nodeIndex, e.what());
            SPDLOG_ERROR(error);
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

std::vector<Light> GLTF_Loader::LoadSceneLights(size_t sceneIndex, const glm::mat4& transform)
{
    const auto& sceneData = gltfData_["scenes"][sceneIndex];

    std::vector<Light> lights;
    for (const auto& nodeIndex : sceneData["nodes"])
    {
        auto nodeInstances = LoadNodeLights(nodeIndex.get<size_t>(), transform);
        lights.insert(lights.end(), std::make_move_iterator(nodeInstances.begin()),
            std::make_move_iterator(nodeInstances.end()));
    }

    return lights;
}

std::vector<Camera> GLTF_Loader::LoadNodeCameras(size_t nodeIndex, const glm::mat4& transform) const
{
    std::vector<Camera> cameras;
    const auto& nodeData = gltfData_["nodes"][nodeIndex];
    glm::mat4 worldTransform = transform * ComputeNodeTransform(nodeIndex);

    if (nodeData.contains("camera"))
    {
        try
        {
            size_t cameraIndex = nodeData["camera"].get<size_t>();
            Camera camera = LoadCamera(cameraIndex);
            camera.SetCameraToWorld(worldTransform);
            cameras.push_back(camera);

            SPDLOG_DEBUG("Loaded camera {} on node {} ({})", cameraIndex,
                nodeData.value("name", "Unnamed_Node"), nodeIndex);
        }
        catch (const std::exception& e)
        {
            const auto error =
                std::format("Failed to load camera on node {}: {}", nodeIndex, e.what());
            SPDLOG_ERROR(error);
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

std::vector<Camera> GLTF_Loader::LoadSceneCameras(
    size_t sceneIndex, const glm::mat4& transform) const
{
    std::vector<Camera> cameras;
    const auto& sceneData = gltfData_["scenes"][sceneIndex];

    for (const auto& nodeIndex : sceneData["nodes"])
    {
        auto nodeCameras = LoadNodeCameras(nodeIndex.get<size_t>(), transform);
        cameras.insert(cameras.end(), std::make_move_iterator(nodeCameras.begin()),
            std::make_move_iterator(nodeCameras.end()));
    }

    return cameras;
}

Camera GLTF_Loader::LoadSceneCamera(size_t sceneIndex, const glm::mat4& transform) const
{
    const auto cameras = LoadSceneCameras(sceneIndex, transform);

    if (cameras.empty()) throw std::runtime_error("No camera found in the scene");

    if (cameras.size() > 1)
        SPDLOG_WARN("Multiple cameras found in the scene, using the first one loaded");

    return cameras.front();
}

template <typename Tm, typename Tl>
static auto CleanupMap(Tm& map, Tl check)
{
    Tm cleanedMap;
    std::copy_if(map.begin(), map.end(), std::inserter(cleanedMap, cleanedMap.end()), check);
    map = std::move(cleanedMap);
}

void GLTF_Loader::Cleanup()
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

    SPDLOG_INFO(
        "GLTF Loader cleaned up unused resources, dropped {} buffers, {} materials, {} images, and "
        "{} meshes",
        droppedBuffers, initialMaterialCount - loadedMaterials_.size(),
        initialImagesCount - loadedImages_.size(), initialMeshCount - loadedMeshes_.size());
}
