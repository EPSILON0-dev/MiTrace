#include "ObjectBVH.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <limits>

#include "Mesh.hpp"

using namespace Scene;

static constexpr uint32_t maxInstancesPerLeaf = 1;
static constexpr uint32_t maxBuildDepth = 32;

uint32_t ObjectBVH::BuildRecursive(std::vector<ObjectInstance> items, uint32_t depth)
{
    const auto nodeIndex = static_cast<uint32_t>(nodes_.size());
    nodes_.emplace_back();

    glm::vec3 min(std::numeric_limits<float>::max());
    glm::vec3 max(std::numeric_limits<float>::lowest());
    glm::vec3 centroidMin(std::numeric_limits<float>::max());
    glm::vec3 centroidMax(std::numeric_limits<float>::lowest());
    for (const auto& item : items)
    {
        min = glm::min(min, item.min);
        max = glm::max(max, item.max);
        centroidMin = glm::min(centroidMin, item.centroid);
        centroidMax = glm::max(centroidMax, item.centroid);
    }
    nodes_[nodeIndex].SetAABB({min, max});

    const glm::vec3 extent = centroidMax - centroidMin;
    const bool degenerate = extent.x <= 0.0f && extent.y <= 0.0f && extent.z <= 0.0f;
    if (items.size() <= maxInstancesPerLeaf || depth == 0 || degenerate)
    {
        const auto first = static_cast<uint32_t>(instanceIndices_.size());
        for (const auto& item : items) instanceIndices_.push_back(item.instanceIndex);
        nodes_[nodeIndex].SetLeaf(first, static_cast<uint32_t>(items.size()));
        return nodeIndex;
    }

    int axis = 0;
    if (extent.y > extent.x && extent.y > extent.z)
        axis = 1;
    else if (extent.z > extent.x && extent.z > extent.y)
        axis = 2;

    const auto mid = static_cast<int>(items.size() / 2);
    std::nth_element(items.begin(), items.begin() + mid, items.end(),
        [axis](const ObjectInstance& a, const ObjectInstance& b)
        { return a.centroid[axis] < b.centroid[axis]; });
    std::vector<ObjectInstance> rightItems(items.begin() + mid, items.end());
    items.erase(items.begin() + mid, items.end());

    const uint32_t leftChild = BuildRecursive(std::move(items), depth - 1);
    const uint32_t rightChild = BuildRecursive(std::move(rightItems), depth - 1);
    nodes_[nodeIndex].SetInnerNode(leftChild, rightChild);
    return nodeIndex;
}

ObjectBVH::ObjectBVH(const std::vector<MeshInstance>& instances)
{
    std::vector<ObjectInstance> items;
    items.reserve(instances.size());
    for (size_t i = 0; i < instances.size(); ++i)
    {
        const auto& aabb = instances[i].GetWorldAABB();
        items.push_back(
            {aabb.first, aabb.second, (aabb.first + aabb.second) * 0.5f, static_cast<uint32_t>(i)});
    }
    if (!items.empty()) BuildRecursive(std::move(items), maxBuildDepth);

    spdlog::debug(
        "Generated object-level BVH, nodes: {}, instances: {}", nodes_.size(), instances.size());
}
