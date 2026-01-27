/**
 * @file Intersect.hpp
 * 
 * Ray-scene intersection utilities.
 */
#pragma once

#include "Ray.hpp"

bool IntersectRayAABB(const Ray& ray, glm::vec3 boxMin, glm::vec3 boxMax);
