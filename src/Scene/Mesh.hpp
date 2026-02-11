#pragma once

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "BVH.hpp"
#include "Loader/Types.hpp"
#include "Material.hpp"

namespace Scene
{

class Mesh
{
   public:
    struct Flags
    {
        bool HasTangent : 1;
        bool HasTexCoord0 : 1;
        bool HasTexCoord1 : 1;
        bool HasColor0 : 1;
    };

    struct alignas(64) Triangle
    {
        glm::vec3 normal[3];
        glm::vec2 texCoord0[3];
        glm::vec3 tangent[3];
        glm::vec2 texCoord1[3];
        glm::vec4 color0[3];
        // We can fit 24 more bytes here if needed :p
    };

   private:
    std::string name_;
    std::vector<glm::vec3> positions_;
    std::vector<Triangle> triangles_;
    BVH bvh_;
    Flags flags_;

   public:
    Mesh() {}

    Mesh(const Loader::Mesh& mesh);

   public:
    const auto& GetName() const noexcept { return name_; }
    inline const auto& GetPositions() const noexcept { return positions_; }
    inline const auto& GetTriangles() const noexcept { return triangles_; }
    inline const auto& GetFlags() const noexcept { return flags_; }
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
