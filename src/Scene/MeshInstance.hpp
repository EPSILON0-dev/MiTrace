#pragma once

#include <memory>

#include "Material.hpp"
#include "Mesh.hpp"

namespace Scene
{

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

    const Mesh& GetMesh() const noexcept { return *mesh_; }
    const std::shared_ptr<Mesh>& GetMeshPtr() const noexcept { return mesh_; }
    const glm::mat4& GetTransform() const noexcept { return transform_; }
    const std::pair<glm::vec3, glm::vec3>& GetWorldAABB() const noexcept { return worldAABB_; }
    const Material& GetMaterial() const noexcept { return material_; }
};

}  // namespace Scene