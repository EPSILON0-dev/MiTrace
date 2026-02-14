#include "Mesh.hpp"

#include <numeric>

#include "CLI/Config.hpp"

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
    const auto& config = Config::GetConfig();

    name_ = mesh.name;
    positions_ = UnpackVector(mesh.positions, mesh.indices);
    normals_ = UnpackVector(mesh.normals, mesh.indices);
    tangents_ = UnpackVector(mesh.tangents, mesh.indices);
    texCoord0_ = UnpackVector(mesh.texCoord0, mesh.indices);
    texCoord1_ = UnpackVector(mesh.texCoord1, mesh.indices);
    color0_ = UnpackVector(mesh.color0, mesh.indices);

    bool generateBVH = !config.rendering.disableBVH;
    generateBVH &= GetTriangleCount() > config.rendering.maxBVHTriangles;
    // TODO Do something better than a fixed value here
    if (generateBVH) bvh_ = BVH(*this, config.rendering.maxBVHTriangles, 30);
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
