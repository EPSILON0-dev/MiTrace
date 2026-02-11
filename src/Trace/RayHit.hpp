#pragma once

#include "Ray.hpp"
#include "Scene/Mesh.hpp"

// Forward declaration
namespace Scene
{
class MeshInstance;
}

class RayHit;
class RayHitGeometryInfo;

class RayHit : public Ray
{
   public:
    const Scene::MeshInstance* meshInstance;
    glm::vec2 baryCoord;
    glm::vec3 worldPosition;
    size_t triangleIndex;
    float distance;

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
    Scene::Mesh::Flags Flags;

    glm::vec3 Normal;
    glm::vec4 Tangent;
    glm::vec2 TexCoord0;
    glm::vec2 TexCoord1;
    glm::vec3 Color0;

    RayHitGeometryInfo(const RayHit& rayHit) noexcept;
};
