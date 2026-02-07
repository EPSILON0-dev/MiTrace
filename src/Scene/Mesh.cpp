#include "Mesh.hpp"

#include <numeric>

using namespace Scene;

template <typename Td, typename Ti>
static std::vector<Td> UnpackVector(const std::vector<Td>& data, const std::vector<Ti>& indices)
{
    // Skip unpacking if there are no indices
    if (data.empty()) return {};

    // Skip unpacking empty vectors
    if (indices.empty()) return data;

    std::vector<Td> unpacked;
    unpacked.reserve(indices.size());
    for (const auto& index : indices) unpacked.push_back(data[index]);
    return unpacked;
}

Mesh::Mesh(const Loader::Mesh& mesh)
{
    name_ = mesh.name;
    positions_ = UnpackVector(mesh.positions, mesh.indices);
    normals_ = UnpackVector(mesh.normals, mesh.indices);
    tangents_ = UnpackVector(mesh.tangents, mesh.indices);
    texCoord0_ = UnpackVector(mesh.texCoord0, mesh.indices);
    texCoord1_ = UnpackVector(mesh.texCoord1, mesh.indices);
    color0_ = UnpackVector(mesh.color0, mesh.indices);
    CalculateAABB();
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
