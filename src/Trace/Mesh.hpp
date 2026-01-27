/**
 * @file Mesh.hpp
 *
 * Mesh representation containing vertex attributes, indices, and material.
 */
#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <string>

#include "Ray.hpp"
#include "RayHit.hpp"

class Mesh
{
    friend class GLTF;
    using MeshHash = uint32_t;

   private:
    std::string name_;
    std::vector<glm::vec3> positions_;  // REQUIRED
    std::vector<glm::vec3> normals_;    // REQUIRED
    std::vector<glm::vec4> tangents_;   // OPTIONAL
    std::vector<glm::vec2> texCoord0_;  // OPTIONAL (Main texture)
    std::vector<glm::vec2> texCoord1_;  // OPTIONAL (Reserved for future use, e.g. lightmaps)
    std::vector<glm::vec4> color0_;     // OPTIONAL
    std::vector<uint32_t> indices_;     // REQUIRED
    mutable std::optional<MeshHash> meshHash_ = std::nullopt;

   public:
    Mesh() noexcept {}
    virtual ~Mesh();

   public:
    const std::string& GetName() const noexcept { return name_; }
    const std::vector<glm::vec3>& GetPositions() const noexcept { return positions_; }
    const std::vector<glm::vec3>& GetNormals() const noexcept { return normals_; }
    const std::vector<glm::vec4>& GetTangents() const noexcept { return tangents_; }
    const std::vector<glm::vec2>& GetTexCoord0() const noexcept { return texCoord0_; }
    const std::vector<glm::vec2>& GetTexCoord1() const noexcept { return texCoord1_; }
    const std::vector<glm::vec4>& GetColor0() const noexcept { return color0_; }
    const std::vector<uint32_t>& GetIndices() const noexcept { return indices_; }

   public:
    MeshHash GetMeshHash() const noexcept;
    bool operator==(const Mesh& other) const noexcept;

    glm::vec3 CalculateAABBMin() const noexcept;
    glm::vec3 CalculateAABBMax() const noexcept;

    std::optional<RayHit> Intersect(const Ray& ray, const glm::mat4& modelToWorld) const noexcept;
};
