#pragma once

#include <optional>

#include "Ray.hpp"
#include "Scene/Scene.hpp"

namespace BasicBackend
{
std::optional<RayHit> IntersectScene(
    const Ray& ray, const Scene::Scene& scene, bool anyHit) noexcept;
};  // namespace BasicBackend
