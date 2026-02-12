#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Loader
{

enum class SamplerFilter : uint8_t
{
    Nearest,
    Linear
};

enum class TransparencyMode : uint8_t
{
    Opaque,
    Mask,
    Blend
};

enum class LightType : uint8_t
{
    Point,
    Directional,
    Spot,
    Area
};

enum class CameraType : uint8_t
{
    Perspective,
    Orthogonal,
};

struct Mesh
{
    std::string name;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec4> tangents;
    std::vector<glm::vec2> texCoord0;
    std::vector<glm::vec2> texCoord1;
    std::vector<glm::vec4> color0;
    std::vector<uint32_t> indices;
};

struct Image
{
    int width;
    int height;
    int channels;
    std::string name;
    std::shared_ptr<uint8_t[]> pixels;
};

struct Texture
{
    std::shared_ptr<Image> image;
    SamplerFilter filter;
};

struct Material
{
    std::string name;
    Texture baseColorTexture;
    Texture metallicRoughnessTexture;
    Texture normalTexture;
    Texture occlusionTexture;
    Texture emissiveTexture;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    glm::vec3 emissiveFactor = glm::vec3(0.0f);
    float normalScale = 1.0f;
    float metallicFactor = 0.5f;
    float roughnessFactor = 0.5f;
    float occlusionStrength = 0.0f;
    float alphaCutoff = 0.5f;
    TransparencyMode transparencyMode = TransparencyMode::Opaque;
    bool doubleSided = false;
};

struct MeshInstance
{
    std::shared_ptr<Mesh> mesh;
    Material material;
    glm::mat4 transform;
};

struct Light
{
    LightType type;
    glm::mat4 transform;
    glm::vec3 color;
    float intensity;
    float range;
    float pointSize;
    float directionalAngle;
    float spotInnerConeAngle;
    float spotOuterConeAngle;
    glm::vec2 areaSize;
};

struct Camera
{
    CameraType type;
    glm::mat4 cameraToWorld;
    float perspectiveFovY;
    float orthogonalSizeY;
    float aspectRatio;
};

struct Scene
{
    std::vector<MeshInstance> meshInstances;
    std::vector<Light> lights;
    std::optional<Texture> environmentTexture;
    Camera camera;
};

}  // namespace Loader
