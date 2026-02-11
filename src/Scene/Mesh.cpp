#include "Mesh.hpp"

#include <numeric>

#include "Loader/Config.hpp"

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

static inline glm::vec3 ConvertTangent4To3(const glm::vec4& tangent)
{
    // FIXME: It that's what we're supposed to do?
    return glm::normalize(glm::vec3(tangent) * tangent.w);
}

template <typename T>
static inline T ProtectedAccess(const std::vector<T>& data, size_t index)
{
    if (data.empty()) return T{};
    if (index < data.size()) return data[index];
    throw std::out_of_range("Index out of range in ProtectedAccess");
}

static std::vector<Mesh::Triangle> UnpackTriangles(const std::vector<glm::vec3>& normals,
    const std::vector<glm::vec4>& tangents, const std::vector<glm::vec2>& texCoord0,
    const std::vector<glm::vec2>& texCoord1, const std::vector<glm::vec4>& color0,
    const std::vector<uint32_t>& indices)
{
    std::vector<Mesh::Triangle> triangles;
    triangles.reserve(indices.size() / 3);
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        Mesh::Triangle triangle;
        for (int j = 0; j < 3; ++j)
        {
            triangle.normal[j] = ProtectedAccess(normals, indices[i + j]);
            triangle.tangent[j] = ConvertTangent4To3(ProtectedAccess(tangents, indices[i + j]));
            triangle.texCoord0[j] = ProtectedAccess(texCoord0, indices[i + j]);
            triangle.texCoord1[j] = ProtectedAccess(texCoord1, indices[i + j]);
            triangle.color0[j] = ProtectedAccess(color0, indices[i + j]);
        }
        triangles.push_back(triangle);
    }
    return triangles;
}

static Mesh::Flags ExtractFlags(const std::vector<glm::vec4>& tangents,
    const std::vector<glm::vec2>& texCoord0, const std::vector<glm::vec2>& texCoord1,
    const std::vector<glm::vec4>& color0)
{
    Mesh::Flags flags{};
    flags.HasTangent = !tangents.empty();
    flags.HasTexCoord0 = !texCoord0.empty();
    flags.HasTexCoord1 = !texCoord1.empty();
    flags.HasColor0 = !color0.empty();
    return flags;
}

Mesh::Mesh(const Loader::Mesh& mesh)
{
    const auto& config = Config::GetConfig();

    name_ = mesh.name;
    positions_ = UnpackVector(mesh.positions, mesh.indices);

    triangles_ = UnpackTriangles(
        mesh.normals, mesh.tangents, mesh.texCoord0, mesh.texCoord1, mesh.color0, mesh.indices);

    flags_ = ExtractFlags(mesh.tangents, mesh.texCoord0, mesh.texCoord1, mesh.color0);

    bvh_ = BVH(*this, config.rendering.maxBVHTriangles, 30);
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
