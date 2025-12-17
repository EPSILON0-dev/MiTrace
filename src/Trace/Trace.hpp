#pragma once

#include "Camera.hpp"  // IWYU pragma: keep
#include "Ray.hpp"
#include "RayHit.hpp"  // IWYU pragma: keep
#include "Scene.hpp"

namespace Trace
{
// TODO
glm::u8vec4 TraceScene(const Ray& ray, const Scene& scene);
};  // namespace Trace
