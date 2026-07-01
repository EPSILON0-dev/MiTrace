#pragma once
#include <glm/glm.hpp>

// Forward declaration
namespace Scene
{
class MeshInstance;
}

namespace Tracer
{

class Ray
{
   public:
    glm::vec3 origin;
    glm::vec3 direction;

   public:
    Ray() noexcept : origin(0.0f), direction(0.0f) {}
    Ray(const glm::vec3& orig, const glm::vec3& dir) noexcept : origin(orig), direction(dir) {}
};

class RayHit : public Ray
{
   public:
    const Scene::MeshInstance* meshInstance;
    glm::vec2 baryCoord;
    glm::vec3 worldPosition;
    float distance;
    uint32_t triangleIndex;
    uint32_t bvIndex;
    uint32_t bvhTests;
    uint32_t triangleTests;

   public:
    RayHit() noexcept : distance(-1.0f) {}
    RayHit(const Ray& ray, float t_val) noexcept : Ray(ray), distance(t_val) {}
    RayHit(const glm::vec3& orig, const glm::vec3& dir, float t_val) noexcept
        : Ray(orig, dir), distance(t_val)
    {
    }
};

class RayHitGeometryInfo : public RayHit
{
   public:
    struct Flags
    {
        bool HasTangent : 1;
        bool HasTexCoord0 : 1;
        bool HasTexCoord1 : 1;
        bool HasColor0 : 1;
    } Flags;

    glm::vec3 Normal;
    glm::vec4 Tangent;
    glm::vec2 TexCoord0;
    glm::vec2 TexCoord1;
    glm::vec3 Color0;

    RayHitGeometryInfo(const RayHit& rayHit) noexcept;
};

}  // namespace Tracer
