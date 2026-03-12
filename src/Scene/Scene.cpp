#include "Scene.hpp"

#include <map>
#include <mutex>
#include <set>

#include "CLI/Config.hpp"
#include "Common/ParallelForEach.hpp"

// This has to be done like this so that meshes are not duplicated when multiple mesh instances
// reference the same mesh.
void Scene::Scene::CopyLoaderMeshInstances(const Loader::Scene& scene)
{
    meshInstances_.reserve(meshInstances_.size() + scene.meshInstances.size());

    // Gather all distinct meshes
    std::set<const Loader::Mesh*> meshSet;
    for (const auto& meshInstance : scene.meshInstances)
    {
        const auto& meshPtr = meshInstance.mesh.get();
        if (!meshSet.contains(meshPtr)) meshSet.emplace(meshPtr);
    }

    // Convert all the loader meshes into scene meshes in parallel
    std::mutex mapMutex;
    std::map<const Loader::Mesh*, std::shared_ptr<Mesh>> meshMap;
    const auto nThread = Config::GetConfig().rendering.numThreads;
    Common::ParallelForEach(meshSet.begin(), meshSet.end(), nThread,
        [&mapMutex, &meshMap](const auto meshPtr)
        {
            auto mesh = std::make_shared<Mesh>(*meshPtr);
            std::scoped_lock<std::mutex> lock(mapMutex);
            meshMap[meshPtr] = mesh;
        });

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
    Texture::LoadCachedImages();
    spdlog::info("Scene loaded.");
}
