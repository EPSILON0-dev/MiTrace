#pragma once

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <random>

#include "RayHit.hpp"

class MaterialBase
{
   public:
    MaterialBase() = default;
    virtual ~MaterialBase() = default;

   public:
    // Virtual method to access or use material properties
    // For now we'll only have reflect as an example
    virtual void Reflect(const RayHitGeometryInfo& geomInfo, glm::vec3& viewVec,
        glm::vec3& energyMultiplier, std::mt19937& rng) const noexcept = 0;
    virtual glm::vec3 ComputeLightContribution(const RayHitGeometryInfo& geomInfo,
        const glm::vec3& viewVec, const glm::vec3& lightVec,
        const glm::vec3& lightColor) const noexcept = 0;
};
