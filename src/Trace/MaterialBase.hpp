#pragma once

#include <glm/glm.hpp>

#include "RayHit.hpp"

class MaterialBase
{
   public:
    MaterialBase() = default;
    virtual ~MaterialBase() = default;

   public:
    // Virtual method to access or use material properties
    // For now we'll only have reflect as an example
    virtual void Reflect(const RayHitGeometryInfo& geomInfo, glm::vec3& direction,
        glm::vec3& energy) const noexcept = 0;
};
