#pragma once

#include <optional>
#include <random>

#include "Ray.hpp"
#include "Scene/Scene.hpp"

namespace Tracer
{
std::optional<RayHit> IntersectScene(const Ray& ray, const Scene::Scene& scene, std::mt19937& rng,
    size_t& raysTraced) noexcept;
};  // namespace Tracer
