#include "Mesh.hpp"

#include <cstddef>
#include <numeric>
#include <spdlog/spdlog.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>

Mesh::~Mesh()
{
    spdlog::trace("Destroying Mesh '{}'", name_);
}

glm::vec3 Mesh::CalculateAABBMin() const noexcept
{
    return std::accumulate(positions_.begin(), positions_.end(),
        glm::vec3(std::numeric_limits<float>::max()),
        [](const auto& a, const auto& b) { return glm::min(a, b); });
}

glm::vec3 Mesh::CalculateAABBMax() const noexcept
{
    return std::accumulate(positions_.begin(), positions_.end(),
        glm::vec3(std::numeric_limits<float>::lowest()),
        [](const auto& a, const auto& b) { return glm::max(a, b); });
}

std::optional<RayHit> Mesh::Intersect(const Ray& ray, const glm::mat4& modelToWorld) const noexcept
{
    // Transform the ray to local space
    Ray localRay;
    auto invModelToWorld = glm::inverse(modelToWorld);
    localRay.origin = glm::vec3(invModelToWorld * glm::vec4(ray.origin, 1.0f));
    localRay.direction =
        glm::normalize(glm::vec3(invModelToWorld * glm::vec4(ray.direction, 0.0f)));

    float dist = std::numeric_limits<float>::max();
    glm::vec2 baryCoord = glm::vec2(0.0f);
    size_t triangleIndex = 0;

    for (size_t i = 0; i < indices_.size(); i += 3)
    {
        const glm::vec3& v0 = positions_[indices_[i + 0]];
        const glm::vec3& v1 = positions_[indices_[i + 1]];
        const glm::vec3& v2 = positions_[indices_[i + 2]];

        float currentDistance;
        glm::vec2 currentBaryCoord;

        if (glm::intersectRayTriangle(
                localRay.origin, localRay.direction, v0, v1, v2, currentBaryCoord, currentDistance))
        {
            if (currentDistance > 0.0f && currentDistance < dist)
            {
                dist = currentDistance;
                baryCoord = currentBaryCoord;
                triangleIndex = i / 3;
            }
        }
    }

    if (dist < std::numeric_limits<float>::max())
    {
        RayHit hit;
        hit.worldPosition =
            glm::vec3(modelToWorld * glm::vec4(localRay.origin + dist * localRay.direction, 1.0f));
        hit.distance = glm::length(hit.worldPosition - ray.origin);
        hit.triangleIndex = triangleIndex;
        hit.baryCoord = baryCoord;

        return hit;
    }
    else
    {
        return std::nullopt;
    }
}

uint32_t Mesh::GetMeshHash() const noexcept
{
    if (meshHash_.has_value()) return meshHash_.value();

    uint32_t hash = 0;

    auto hashCombine = [&hash](uint32_t value)
    { hash ^= value + 0x9e3779b9 + (hash << 6) + (hash >> 2); };

    auto hashCombineVec = [&hashCombine](const auto& vec)
    {
        for (size_t i = 0; i < sizeof(vec) / sizeof(vec[0]); ++i)
            hashCombine(std::hash<float>()(vec[0]));
    };

    for (const auto& item : positions_) hashCombineVec(item);
    for (const auto& item : normals_) hashCombineVec(item);
    for (const auto& item : tangents_) hashCombineVec(item);
    for (const auto& item : texCoord0_) hashCombineVec(item);
    for (const auto& item : texCoord1_) hashCombineVec(item);
    for (const auto& item : color0_) hashCombineVec(item);
    for (const auto& item : indices_) hashCombine(item);

    meshHash_ = hash;
    return hash;
}

bool Mesh::operator==(const Mesh& other) const noexcept
{
    // Compare everything except the name
    return other.positions_ == this->positions_ && other.normals_ == this->normals_ &&
           other.tangents_ == this->tangents_ && other.texCoord0_ == this->texCoord0_ &&
           other.texCoord1_ == this->texCoord1_ && other.color0_ == this->color0_ &&
           other.indices_ == this->indices_;
}
