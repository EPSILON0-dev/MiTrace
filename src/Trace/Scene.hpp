/**
 * @file Scene.hpp
 * 
 * Scene representation containing mesh instances, lights, and environment
 * texture (skybox).
 */
#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "Trace/Light.hpp"
#include "Trace/MeshInstance.hpp"
#include "Trace/Texture.hpp"

class Scene
{
   private:
    std::vector<MeshInstance> meshInstances_;
    std::vector<Light> lights_;
    std::optional<Texture> environmentTexture_;

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

    void AddLight(const Light& light) noexcept { lights_.push_back(light); }

    void AddLights(const std::vector<Light>& lights) noexcept
    {
        lights_.insert(lights_.end(), lights.begin(), lights.end());
    }

    void SetEnvironmentTexture(const Texture& texture) noexcept
    {
        environmentTexture_ = texture;
    }

    const std::vector<MeshInstance>& GetMeshInstances() const noexcept { return meshInstances_; }
    const std::vector<Light>& GetLights() const noexcept { return lights_; }
    const std::optional<Texture>& GetEnvironmentTexture() const noexcept
    {
        return environmentTexture_;
    }
};
