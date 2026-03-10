#pragma once

#include <glm/glm.hpp>
#include <string>

#include "BVH.hpp"
#include "Material.hpp"
#include "Loader/Types.hpp"

namespace Scene
{

class Mesh
{
   private:
    std::string name_;
    std::vector<glm::vec3> positions_;
    std::vector<glm::vec3> normals_;
    // std::vector<glm::vec4> tangents_;
    std::vector<glm::vec2> texCoord0_;
    std::vector<glm::vec2> texCoord1_;
    std::vector<glm::vec4> color0_;
    std::optional<BVH> bvh_;

   public:
    Mesh() = default;
    Mesh(const Loader::Mesh& mesh);

   public:
    const auto& GetName() const noexcept { return name_; }
    const auto& GetPositions() const noexcept { return positions_; }
    const auto& GetNormals() const noexcept { return normals_; }
    // const auto& GetTangents() const noexcept { return tangents_; }
    const auto& GetTexCoord0() const noexcept { return texCoord0_; }
    const auto& GetTexCoord1() const noexcept { return texCoord1_; }
    const auto& GetColor0() const noexcept { return color0_; }
    const auto& GetBVH() const noexcept { return bvh_.value(); }
    bool HasBVH() const noexcept { return bvh_.has_value(); }

    auto GetTriangleCount() const noexcept { return positions_.size() / 3; }

   public:
    std::pair<glm::vec3, glm::vec3> CalculateAABB() const noexcept;
};

class MeshInstance
{
   private:
    std::shared_ptr<Mesh> mesh_;
    Material material_;
    glm::mat4 transform_;
    std::pair<glm::vec3, glm::vec3> worldAABB_;

   private:
    void CalculateWorldAABB();

   public:
    MeshInstance() = default;

    MeshInstance(std::shared_ptr<Mesh> mesh, Material material, const glm::mat4& transform)
        : mesh_(std::move(mesh)), material_(std::move(material)), transform_(transform)
    {
        CalculateWorldAABB();
    }

    // Mesh Instances have to be done explicitly, so we can avoid unnecessary duplication of Meshes
    MeshInstance(const Loader::MeshInstance& meshInstance) = delete;

    const Mesh& GetMesh() const noexcept { return *mesh_; }
    const std::shared_ptr<Mesh>& GetMeshPtr() const noexcept { return mesh_; }
    const glm::mat4& GetTransform() const noexcept { return transform_; }
    const Material& GetMaterial() const noexcept { return material_; }
    const std::pair<glm::vec3, glm::vec3>& GetWorldAABB() const noexcept
    {
        return worldAABB_;
    }
};

}  // namespace Scene
