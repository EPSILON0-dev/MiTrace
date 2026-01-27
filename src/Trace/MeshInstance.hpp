/**
 * @file MeshInstance.hpp
 *
 * Represents an instance of a mesh in the scene with its own transformation
 * and bounding box
 */
#pragma once

#include <memory>
#include <optional>

#include "Material.hpp"
#include "Mesh.hpp"
#include "Ray.hpp"
#include "RayHit.hpp"

class MeshInstance
{
   private:
    std::shared_ptr<Mesh> mesh_;
    glm::mat4 transform_;
    glm::vec3 worldAABBMin_;
    glm::vec3 worldAABBMax_;
    Material material_;

private:
    void CalculateWorldAABB();

   public:
    MeshInstance(std::shared_ptr<Mesh> mesh, const Material &material, const glm::mat4& transform);

    const Mesh& GetMesh() const noexcept { return *mesh_; }
    const std::shared_ptr<Mesh>& GetMeshPtr() const noexcept { return mesh_; }
    const glm::mat4& GetTransform() const noexcept { return transform_; }
    const glm::vec3& GetWorldAABBMin() const noexcept { return worldAABBMin_; }
    const glm::vec3& GetWorldAABBMax() const noexcept { return worldAABBMax_; }
    const Material& GetMaterial() const noexcept { return material_; }

    void SetMaterial(const Material& material) { material_ = material; }
    void SetTransform(const glm::mat4& transform);
    void SetMesh(const std::shared_ptr<Mesh>& mesh);

    std::optional<RayHit> IntersectRay(const Ray& ray) const;
};
