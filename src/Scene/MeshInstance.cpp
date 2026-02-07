#include "MeshInstance.hpp"

using namespace Scene;

void MeshInstance::CalculateWorldAABB()
{
    auto aabb = mesh_->CalculateAABB();

    glm::vec3 corners[8] = {
        {aabb.first.x, aabb.first.y, aabb.first.z},
        {aabb.first.x, aabb.first.y, aabb.second.z},
        {aabb.first.x, aabb.second.y, aabb.first.z},
        {aabb.first.x, aabb.second.y, aabb.second.z},
        {aabb.second.x, aabb.first.y, aabb.first.z},
        {aabb.second.x, aabb.first.y, aabb.second.z},
        {aabb.second.x, aabb.second.y, aabb.first.z},
        {aabb.second.x, aabb.second.y, aabb.second.z},
    };

    worldAABB_.first = glm::vec3(std::numeric_limits<float>::max());
    worldAABB_.second = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& corner : corners)
    {
        glm::vec4 transformedCorner = transform_ * glm::vec4(corner, 1.0f);
        worldAABB_.first = glm::min(worldAABB_.first, glm::vec3(transformedCorner));
        worldAABB_.second = glm::max(worldAABB_.second, glm::vec3(transformedCorner));
    }
}