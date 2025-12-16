#include "Intersect.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>

bool IntersectRayAABB(const Ray& ray, glm::vec3 boxMin, glm::vec3 boxMax)
{
    glm::vec3 invDir = 1.0f / ray.direction;
    glm::vec3 t1 = (boxMin - ray.origin) * invDir;
    glm::vec3 t2 = (boxMax - ray.origin) * invDir;
    glm::vec3 tmin3 = glm::min(t1, t2);
    glm::vec3 tmax3 = glm::max(t1, t2);

    return glm::compMax(tmin3) <= glm::compMin(tmax3);
}
