#include "Scene.hpp"

#include <spdlog/spdlog.h>

#include "Mesh.hpp"

void Scene::AddMeshInstance(const MeshInstance& meshInstance) noexcept
{
    meshInstances_.push_back(meshInstance);
}

void Scene::AddMeshInstances(const std::vector<MeshInstance>& meshInstances) noexcept
{
    for (const auto& instance : meshInstances) AddMeshInstance(instance);
}

void Scene::AddLight(const Light& light) noexcept
{
    lights_.push_back(light);
}

void Scene::AddLights(const std::vector<Light>& lights) noexcept
{
    lights_.insert(lights_.end(), lights.begin(), lights.end());
}

void Scene::MergeEquivalentMeshes()
{
    // There's probably a better way to do that, but I don't believe in that "no raw loops" crap
    // they feed us (it's a skill issue)
    // TODO Make this more cpp-ish later
    auto& instVec = meshInstances_;
    for (size_t i = 0; i < instVec.size(); ++i)
    {
        for (size_t j = i + 1; j < instVec.size(); ++j)
        {
            auto& instA = instVec[i];
            auto& instB = instVec[j];

            // Skip if these are instances of the same mesh
            if (instA.GetMeshPtr() == instB.GetMeshPtr()) continue;
            // Skip if hashes differ
            if (instA.GetMesh().GetMeshHash() != instB.GetMesh().GetMeshHash()) continue;
            // Finally, do a full comparison
            if (instA.GetMesh() != instB.GetMesh()) continue;

            // They are equivalent, merge them. Set the latter one to the former's mesh to reduce
            // future comparisons
            instB.SetMesh(instA.GetMeshPtr());
            spdlog::trace("Merged equivalent meshes: '{}' and '{}'", instA.GetMesh().GetName(),
                instB.GetMesh().GetName());
        }
    }

    // Effectively does nothing, still no harm in keeping it, at least for now
}

void Scene::Optimize()
{
    MergeEquivalentMeshes();
    spdlog::info("Scene optimization completed.");
}
