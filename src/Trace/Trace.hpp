#pragma once

#include "Camera.hpp"  // IWYU pragma: keep
#include "Ray.hpp"
#include "RayHit.hpp"  // IWYU pragma: keep
#include "Scene/Scene.hpp"

namespace Trace
{
// TODO
bool TraceScene(const Ray& ray, const Scene& scene);
};  // namespace Trace
