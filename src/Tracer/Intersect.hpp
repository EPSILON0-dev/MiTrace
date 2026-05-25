#pragma once

#include <optional>

#include "Ray.hpp"
#include "Scene/Scene.hpp"

namespace Tracer
{
std::optional<RayHit> IntersectScene(const Ray& ray, const Scene::Scene& scene) noexcept;
};  // namespace Tracer
