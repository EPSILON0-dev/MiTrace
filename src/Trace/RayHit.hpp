#pragma once

#include "Ray.hpp"

class RayHit : public Ray
{
   public:
    glm::vec3 position;
    glm::vec3 normal;
    float distance;

   public:
    RayHit() noexcept : position(0.0f), normal(0.0f), distance(-1.0f) {}
    RayHit(const Ray& ray, const glm::vec3& pos, const glm::vec3& norm, float t_val) noexcept
        : Ray(ray), position(pos), normal(norm), distance(t_val)
    {
    }
    RayHit(const glm::vec3& orig, const glm::vec3& dir, const glm::vec3& pos, const glm::vec3& norm,
        float t_val) noexcept
        : Ray(orig, dir), position(pos), normal(norm), distance(t_val)
    {
    }
};
