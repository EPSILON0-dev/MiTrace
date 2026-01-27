/**
 * @file MeshInstance.hpp
 * 
 * Represents an instance of a mesh in the scene with its own transformation
 * and bounding box
 */
#pragma once

#include <optional>

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

   public:
    MeshInstance(std::shared_ptr<Mesh> mesh, const glm::mat4& transform);

    const Mesh& GetMesh() const noexcept { return *mesh_; }
    const glm::mat4& GetTransform() const noexcept { return transform_; }
    const glm::vec3& GetWorldAABBMin() const noexcept { return worldAABBMin_; }
    const glm::vec3& GetWorldAABBMax() const noexcept { return worldAABBMax_; }

    std::optional<RayHit> IntersectRay(const Ray& ray) const;
};
