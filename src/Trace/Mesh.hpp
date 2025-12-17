#pragma once

#include <cstddef>
#include <glm/glm.hpp>

#include "Ray.hpp"

class Mesh
{
    friend class GLTF_Loader;

   private:
    std::vector<glm::vec3> positions_;  // REQUIRED
    std::vector<glm::vec3> normals_;    // REQUIRED
    std::vector<glm::vec4> tangents_;   // OPTIONAL
    std::vector<glm::vec2> texCoord0_;  // OPTIONAL (Main texture)
    std::vector<glm::vec2> texCoord1_;  // OPTIONAL (Reserved for future use, e.g. lightmaps)
    std::vector<glm::vec4> color0_;     // OPTIONAL
    std::vector<uint32_t> indices_;     // REQUIRED

   public:
    Mesh() noexcept {}

   public:
    const std::vector<glm::vec3>& GetPositions() const noexcept { return positions_; }
    const std::vector<glm::vec3>& GetNormals() const noexcept { return normals_; }
    const std::vector<glm::vec4>& GetTangents() const noexcept { return tangents_; }
    const std::vector<glm::vec2>& GetTexCoord0() const noexcept { return texCoord0_; }
    const std::vector<glm::vec2>& GetTexCoord1() const noexcept { return texCoord1_; }
    const std::vector<glm::vec4>& GetColor0() const noexcept { return color0_; }
    const std::vector<uint32_t>& GetIndices() const noexcept { return indices_; }

   public:
    glm::vec3 GetAABBMin() const noexcept;
    glm::vec3 GetAABBMax() const noexcept;

    bool Intersect(
        const Ray& ray, size_t& triangleIndex, float& dist, glm::vec2& baryCoord) const noexcept;
};
