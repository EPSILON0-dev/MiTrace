/**
 * @file Mesh.hpp
 * 
 * Mesh representation containing vertex attributes, indices, and material.
 */
#pragma once

// TODO Seprate the mesh data and the material data, they're tightly coupled
//      in the GLTF files, but multiple meshes can point to the same accessors
//      and thus effectively be the same mesh. Some optimization can be done
//      to share the mesh data for better cache locality.

#include <glm/glm.hpp>
#include <memory>
#include <optional>

#include "Ray.hpp"
#include "RayHit.hpp"
#include "Trace/Material.hpp"

class Mesh
{
    friend class GLTF_Loader;

   private:
    std::string name_;
    std::vector<glm::vec3> positions_;  // REQUIRED
    std::vector<glm::vec3> normals_;    // REQUIRED
    std::vector<glm::vec4> tangents_;   // OPTIONAL
    std::vector<glm::vec2> texCoord0_;  // OPTIONAL (Main texture)
    std::vector<glm::vec2> texCoord1_;  // OPTIONAL (Reserved for future use, e.g. lightmaps)
    std::vector<glm::vec4> color0_;     // OPTIONAL
    std::vector<uint32_t> indices_;     // REQUIRED
    std::shared_ptr<Material> material_;

   public:
    Mesh() noexcept {}

   public:
    const std::string& GetName() const noexcept { return name_; }
    const std::vector<glm::vec3>& GetPositions() const noexcept { return positions_; }
    const std::vector<glm::vec3>& GetNormals() const noexcept { return normals_; }
    const std::vector<glm::vec4>& GetTangents() const noexcept { return tangents_; }
    const std::vector<glm::vec2>& GetTexCoord0() const noexcept { return texCoord0_; }
    const std::vector<glm::vec2>& GetTexCoord1() const noexcept { return texCoord1_; }
    const std::vector<glm::vec4>& GetColor0() const noexcept { return color0_; }
    const std::vector<uint32_t>& GetIndices() const noexcept { return indices_; }
    const std::shared_ptr<Material>& GetMaterial() const noexcept { return material_; }

   public:
    void SetMaterial(const std::shared_ptr<Material>& material) noexcept { material_ = material; }

   public:
    glm::vec3 GetAABBMin() const noexcept;
    glm::vec3 GetAABBMax() const noexcept;

    std::optional<RayHit> Intersect(const Ray& ray, const glm::mat4& modelToWorld) const noexcept;
};
