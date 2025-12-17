#pragma once

#include <glm/glm.hpp>

class MaterialBase
{
   public:
    struct GeometryInfo
    {
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec2 uv;
    };

   public:
    MaterialBase() = default;
    virtual ~MaterialBase() = default;

   public:
    // Virtual method to access or use material properties
    // For now we'll only have reflect as an example
    virtual void Reflect(const GeometryInfo& geometryInfo, glm::vec3& direction,
        glm::vec3& energy) const noexcept = 0;
};
