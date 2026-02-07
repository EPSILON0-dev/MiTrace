#include "Mesh.hpp"

#include <numeric>

using namespace Scene;

auto Mesh::GetTriangleCount() const noexcept
{
    // If there are no indices, we assume the mesh is non-indexed
    return indices_.empty() ? positions_.size() / 3 : indices_.size() / 3;
}

std::pair<glm::vec3, glm::vec3> Mesh::CalculateAABB() const noexcept
{
    auto min = std::accumulate(positions_.begin(), positions_.end(),
        glm::vec3(std::numeric_limits<float>::max()),
        [](const auto& a, const auto& b) { return glm::min(a, b); });

    auto max = std::accumulate(positions_.begin(), positions_.end(),
        glm::vec3(std::numeric_limits<float>::lowest()),
        [](const auto& a, const auto& b) { return glm::max(a, b); });

    return {min, max};
}
