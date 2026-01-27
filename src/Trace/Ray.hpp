/**
 * @file Ray.hpp
 * 
 * Representation of a ray in 3D space.
 */
#pragma once
#include <glm/glm.hpp>

class Ray
{
   public:
    glm::vec3 origin;
    glm::vec3 direction;

   public:
    Ray() noexcept : origin(0.0f), direction(0.0f) {}
    Ray(const glm::vec3& orig, const glm::vec3& dir) noexcept : origin(orig), direction(dir) {}
};
