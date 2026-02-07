/**
 * @file Intersect.hpp
 *
 * Ray-scene intersection utilities.
 */
#pragma once

#include <optional>

#include "Ray.hpp"
#include "RayHit.hpp"
#include "Scene/MeshInstance.hpp"

bool IntersectRayAABB(const Ray& ray, const std::pair<glm::vec3, glm::vec3>& aabb);
std::optional<RayHit> IntersectMeshInstance(
    const Ray& ray, const Scene::MeshInstance& meshInstance);
