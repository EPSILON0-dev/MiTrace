#pragma once

#include "Ray.hpp"

class MeshInstance;  // Forward declaration

class RayHit : public Ray
{
   public:
    const MeshInstance* meshInstance;
    glm::vec2 baryCoord;
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
