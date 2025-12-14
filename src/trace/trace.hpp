#pragma once

#include "camera.hpp"  // IWYU pragma: keep
#include "ray.hpp"
#include "rayhit.hpp"  // IWYU pragma: keep
#include "scene/scene.hpp"

namespace Trace
{
// TODO
bool TraceScene(const Ray& ray, const Scene& scene);
};  // namespace Trace
