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
    std::vector<glm::vec4> tangents_;
    std::vector<glm::vec2> texCoord0_;
    std::vector<glm::vec2> texCoord1_;
    std::vector<glm::vec4> color0_;
    // Indices are dropped when converting from the Loader Mesh
    // std::vector<uint32_t> indices_;
    BVH bvh_;

   public:
    Mesh() {}

    Mesh(const Loader::Mesh& mesh);

   public:
    const auto& GetName() const noexcept { return name_; }
    inline const auto& GetPositions() const noexcept { return positions_; }
    inline const auto& GetNormals() const noexcept { return normals_; }
    inline const auto& GetTangents() const noexcept { return tangents_; }
    inline const auto& GetTexCoord0() const noexcept { return texCoord0_; }
    inline const auto& GetTexCoord1() const noexcept { return texCoord1_; }
    inline const auto& GetColor0() const noexcept { return color0_; }
    inline const auto& GetBVH() const noexcept { return bvh_; }

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

    MeshInstance(std::shared_ptr<Mesh> mesh, const Material& material, const glm::mat4& transform)
        : mesh_(mesh), material_(material), transform_(transform)
    {
        CalculateWorldAABB();
    }

    // Mesh Instances have to be done explicitly, so we can avoid unnecessary duplication of Meshes
    MeshInstance(const Loader::MeshInstance& meshInstance) = delete;

    inline const Mesh& GetMesh() const noexcept { return *mesh_; }
    inline const std::shared_ptr<Mesh>& GetMeshPtr() const noexcept { return mesh_; }
    inline const glm::mat4& GetTransform() const noexcept { return transform_; }
    inline const Material& GetMaterial() const noexcept { return material_; }
    inline const std::pair<glm::vec3, glm::vec3>& GetWorldAABB() const noexcept
    {
        return worldAABB_;
    }
};

}  // namespace Scene
