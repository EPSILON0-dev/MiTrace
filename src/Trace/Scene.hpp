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

    const std::vector<MeshInstance>& GetMeshInstances() const noexcept
    {
        return meshInstances_;
    }
};
