#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "Trace/MeshInstance.hpp"

class Scene
{
   private:
    std::vector<MeshInstance> meshInstances_;

   public:
    Scene() noexcept {}

    void AddMeshInstance(const MeshInstance& meshInstance) noexcept
    {
        meshInstances_.push_back(meshInstance);
    }

    void AddMeshInstances(const std::vector<MeshInstance>& meshInstances) noexcept
    {
        meshInstances_.insert(meshInstances_.end(), meshInstances.begin(), meshInstances.end());
    }

    const std::vector<MeshInstance>& GetMeshInstances() const noexcept { return meshInstances_; }
};
