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
    const auto& GetName() const noexcept { return name_; }
    const auto& GetPositions() const noexcept { return positions_; }
    const auto& GetNormals() const noexcept { return normals_; }
    const auto& GetTangents() const noexcept { return tangents_; }
    const auto& GetTexCoord0() const noexcept { return texCoord0_; }
    const auto& GetTexCoord1() const noexcept { return texCoord1_; }
    const auto& GetColor0() const noexcept { return color0_; }
    const auto& GetIndices() const noexcept { return indices_; }

    auto GetTriangleCount() const noexcept { return indices_.size() / 3; }

    void SetName(const auto& name) noexcept { name_ = name; }
    void SetPositions(const auto& positions) noexcept { positions_ = positions; }
    void SetNormals(const auto& normals) noexcept { normals_ = normals; }
    void SetTangents(const auto& tangents) noexcept { tangents_ = tangents; }
    void SetTexCoord0(const auto& texCoord0) noexcept { texCoord0_ = texCoord0; }
    void SetTexCoord1(const auto& texCoord1) noexcept { texCoord1_ = texCoord1; }
    void SetColor0(const auto& color0) noexcept { color0_ = color0; }
    void SetIndices(const auto& indices) noexcept { indices_ = indices; }

   public:
    MeshHash GetMeshHash() const noexcept;
    bool operator==(const Mesh& other) const noexcept;

    glm::vec3 CalculateAABBMin() const noexcept;
    glm::vec3 CalculateAABBMax() const noexcept;

    std::optional<RayHit> Intersect(const Ray& ray, const glm::mat4& modelToWorld) const noexcept;
};
