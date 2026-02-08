/**
 * @file Intersect.hpp
 *
 * Ray-scene intersection utilities.
 */
#pragma once

#include <optional>

#include "Ray.hpp"
#include "RayHit.hpp"
#include "Scene/Mesh.hpp"

std::optional<RayHit> IntersectMeshInstance(
    const Ray& ray, const Scene::MeshInstance& meshInstance) noexcept;
