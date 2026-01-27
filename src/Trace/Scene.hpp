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

   private:
    void MergeEquivalentMeshes();

   public:
    Scene() noexcept {}

    void AddMeshInstance(const MeshInstance& meshInstance) noexcept;
    void AddMeshInstances(const std::vector<MeshInstance>& meshInstances) noexcept;
    void AddLight(const Light& light) noexcept;
    void AddLights(const std::vector<Light>& lights) noexcept;

    void SetEnvironmentTexture(const Texture& texture) noexcept { environmentTexture_ = texture; }

    using MeshInstance_vec_cr = const std::vector<MeshInstance>&;
    using Light_vec_cr = const std::vector<Light>&;
    using Texture_opt_cr = const std::optional<Texture>&;

    MeshInstance_vec_cr GetMeshInstances() const noexcept { return meshInstances_; }
    Light_vec_cr GetLights() const noexcept { return lights_; }
    Texture_opt_cr GetEnvironmentTexture() const noexcept { return environmentTexture_; }

    void Optimize();
};
