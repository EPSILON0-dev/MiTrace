#include "Scene.hpp"

#include <map>

// This has to be done like this so that meshes are not duplicated when multiple mesh instances
// reference the same mesh.
void Scene::Scene::CopyLoaderMeshInstances(const Loader::Scene& scene)
{
    meshInstances_.reserve(meshInstances_.size() + scene.meshInstances.size());

    // Gather and convert all Meshes
    std::map<const Loader::Mesh*, std::shared_ptr<Mesh>> meshMap;
    for (const auto& meshInstance : scene.meshInstances)
    {
        const auto meshPtr = meshInstance.mesh.get();
        if (!meshMap.contains(meshPtr))
        {
            meshMap[meshPtr] = std::make_shared<Mesh>(*meshPtr);
        }
    }

    // Populate the mesh instances
    for (const auto& meshInstance : scene.meshInstances)
    {
        meshInstances_.emplace_back(meshMap[meshInstance.mesh.get()],
            Material(meshInstance.material), meshInstance.transform);
    }
}

void Scene::Scene::CopyLoaderLights(const Loader::Scene& scene)
{
    lights_.reserve(lights_.size() + scene.lights.size());
    for (const auto& light : scene.lights)
    {
        lights_.emplace_back(light);
    }
}

Scene::Scene::Scene(const Loader::Scene& scene)
{
    CopyLoaderMeshInstances(scene);
    CopyLoaderLights(scene);
}