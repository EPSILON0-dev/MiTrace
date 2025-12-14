#pragma once

#include "ray.hpp"

class Camera
{
   public:
    glm::vec3 position;
    glm::vec3 forward;
    float yfov;

   public:
    Camera() noexcept
        : position(0.0f),
          forward(0.0f, 0.0f, -1.0f),
          yfov(60.0f)
    {
    }

    Camera(const glm::vec3& pos, const glm::vec3& fwd, float yfov) noexcept
        : position(pos), forward(fwd), yfov(yfov)
    {
    }

   public:
    // UV coordinates are in range [0, 1]
    Ray GenerateRay(float u, float v, float aspectRatio) const noexcept;
};
