#include "BVH.hpp"

#include <spdlog/spdlog.h>

#include <numeric>

#include "Mesh.hpp"

#define DEBUG_BVH_GENERATION 0

using namespace Scene;

BVHTreeNode::BVHTreeNode(const std::vector<glm::vec3>& allTriangles,
    const std::vector<uint32_t>& triangleIndices, int depth)
    : allTriangles_(allTriangles), allTriangleIndices_(triangleIndices), depth_(depth)
{
    ComputeAABB();
}

BVHTreeNode::BVHTreeNode(const std::vector<glm::vec3>& allTriangles,
    const std::vector<uint32_t>& triangleIndices, AABB aabb, int depth)
    : allTriangles_(allTriangles),
      allTriangleIndices_(triangleIndices),
      aabb_(std::move(aabb)),
      depth_(depth)
{
}

void BVHTreeNode::ComputeAABB()
{
    if (allTriangleIndices_.empty())
    {
        throw std::runtime_error("Cannot compute AABB for a BVH node with no triangles.");
    }

    glm::vec3 minPoint(std::numeric_limits<float>::max());
    glm::vec3 maxPoint(std::numeric_limits<float>::lowest());

    for (const auto& index : allTriangleIndices_)
    {
        const auto& v0 = allTriangles_[index * 3 + 0];
        const auto& v1 = allTriangles_[index * 3 + 1];
        const auto& v2 = allTriangles_[index * 3 + 2];

        minPoint = glm::min(minPoint, glm::min(v0, glm::min(v1, v2)));
        maxPoint = glm::max(maxPoint, glm::max(v0, glm::max(v1, v2)));
    }

    aabb_ = {minPoint, maxPoint};
}

glm::vec3 BVHTreeNode::FindCenterOfMass() const
{
    glm::vec3 sum(0.0f);
    for (const auto& index : allTriangleIndices_)
    {
        const auto& v0 = allTriangles_[index * 3 + 0];
        const auto& v1 = allTriangles_[index * 3 + 1];
        const auto& v2 = allTriangles_[index * 3 + 2];
        sum += (v0 + v1 + v2);
    }
    return sum / static_cast<float>(allTriangleIndices_.size() * 3);
}

BVHTreeNode::SplitIndices BVHTreeNode::SplitTriangles(SplitAxis axis, float splitPos) const
{
    std::vector<uint32_t> leftIndices;
    std::vector<uint32_t> rightIndices;

    for (const auto& index : allTriangleIndices_)
    {
        const auto& v0 = allTriangles_[index * 3 + 0];
        const auto& v1 = allTriangles_[index * 3 + 1];
        const auto& v2 = allTriangles_[index * 3 + 2];

        const auto aabbmin = glm::min(v0, glm::min(v1, v2));
        const auto aabbmax = glm::max(v0, glm::max(v1, v2));

        bool inLeft = false;
        bool inRight = false;
        switch (axis)
        {
            case SplitAxis::X:
                inLeft = aabbmin.x < splitPos;
                inRight = aabbmax.x >= splitPos;
                break;
            case SplitAxis::Y:
                inLeft = aabbmin.y < splitPos;
                inRight = aabbmax.y >= splitPos;
                break;
            case SplitAxis::Z:
                inLeft = aabbmin.z < splitPos;
                inRight = aabbmax.z >= splitPos;
                break;
        }

        if (inLeft) leftIndices.push_back(index);
        if (inRight) rightIndices.push_back(index);
    }

    return {leftIndices, rightIndices};
}

// Lower is better
float BVHTreeNode::ComputeSplitScore(SplitIndices& split) const
{
    // Give a huuuge score to splits that don't actually split (all triangles on one side)
    if (split.first.size() == allTriangleIndices_.size() ||
        split.second.size() == allTriangleIndices_.size())
        return std::numeric_limits<float>::max();

    // Triangles in both BVHs / Total triangles
    return static_cast<float>(split.first.size() + split.second.size()) /
           static_cast<float>(allTriangleIndices_.size());
}

void BVHTreeNode::Subdivide(unsigned maxTrianglesPerLeaf, unsigned maxDepth)
{
    // Skip already subdivided nodes or leaf nodes with few triangles
    if (allTriangleIndices_.size() <= static_cast<size_t>(maxTrianglesPerLeaf)) return;

    // Try to split along each axis and choose the best split
    auto centerOfMass = FindCenterOfMass();
    auto xSplit = SplitTriangles(SplitAxis::X, centerOfMass.x);
    auto ySplit = SplitTriangles(SplitAxis::Y, centerOfMass.y);
    auto zSplit = SplitTriangles(SplitAxis::Z, centerOfMass.z);

    auto xScore = ComputeSplitScore(xSplit);
    auto yScore = ComputeSplitScore(ySplit);
    auto zScore = ComputeSplitScore(zSplit);

    if (xScore == std::numeric_limits<float>::max() &&
        yScore == std::numeric_limits<float>::max() && zScore == std::numeric_limits<float>::max())
    {
#if DEBUG_BVH_GENERATION
        spdlog::trace(
            "Failed to subdivide node at depth {}: "
            "no valid splits found, keeping as leaf with {} triangles",
            depth_, allTriangleIndices_.size());
#endif
        return;
    }

    [[maybe_unused]] SplitAxis bestAxis;
    if (xScore < yScore && xScore < zScore)
    {
        bestAxis = SplitAxis::X;
        auto firstAABB = aabb_;
        firstAABB.second.x = centerOfMass.x;
        children_.first =
            std::make_unique<BVHTreeNode>(allTriangles_, xSplit.first, firstAABB, depth_ + 1);
        auto secondAABB = aabb_;
        secondAABB.first.x = centerOfMass.x;
        children_.second =
            std::make_unique<BVHTreeNode>(allTriangles_, xSplit.second, secondAABB, depth_ + 1);
    }
    else if (yScore < zScore)
    {
        bestAxis = SplitAxis::Y;
        auto firstAABB = aabb_;
        firstAABB.second.y = centerOfMass.y;
        children_.first =
            std::make_unique<BVHTreeNode>(allTriangles_, ySplit.first, firstAABB, depth_ + 1);
        auto secondAABB = aabb_;
        secondAABB.first.y = centerOfMass.y;
        children_.second =
            std::make_unique<BVHTreeNode>(allTriangles_, ySplit.second, secondAABB, depth_ + 1);
    }
    else
    {
        bestAxis = SplitAxis::Z;
        auto firstAABB = aabb_;
        firstAABB.second.z = centerOfMass.z;
        children_.first =
            std::make_unique<BVHTreeNode>(allTriangles_, zSplit.first, firstAABB, depth_ + 1);
        auto secondAABB = aabb_;
        secondAABB.first.z = centerOfMass.z;
        children_.second =
            std::make_unique<BVHTreeNode>(allTriangles_, zSplit.second, secondAABB, depth_ + 1);
    }

#if DEBUG_BVH_GENERATION
    spdlog::trace(
        "Subdivided node at depth {} along axis {}: {} triangles on left, {} triangles on right",
        depth_, (bestAxis == SplitAxis::X ? "X" : (bestAxis == SplitAxis::Y ? "Y" : "Z")),
        children_.first->allTriangleIndices_.size(), children_.second->allTriangleIndices_.size());
#endif

    isLeaf_ = false;
    if (maxDepth > 0)
    {
        children_.first->Subdivide(maxTrianglesPerLeaf, maxDepth - 1);
        children_.second->Subdivide(maxTrianglesPerLeaf, maxDepth - 1);
    }
}

unsigned BVHTreeNode::GetMaxDepthRecursive() const
{
    if (IsLeaf()) return 1;
    return 1 + std::max(GetChildren().first->GetMaxDepthRecursive(),
                   GetChildren().second->GetMaxDepthRecursive());
}

BVHTree::BVHTree(
    const std::vector<glm::vec3>& triangles, unsigned maxTrianglesPerLeaf, unsigned maxDepth)
    : allTriangles_(triangles)
{
    std::vector<uint32_t> triangleIndices(triangles.size() / 3);
    std::iota(triangleIndices.begin(), triangleIndices.end(), 0);
    root_ = std::make_unique<BVHTreeNode>(allTriangles_, triangleIndices);
    root_->Subdivide(maxTrianglesPerLeaf, maxDepth - 1);
}

std::pair<size_t, size_t> BVH::AppendIndices(const BVHTreeNode& node)
{
    size_t index = indices_.size();
    indices_.insert(
        indices_.end(), node.GetTriangleIndices().begin(), node.GetTriangleIndices().end());
    size_t count = node.GetTriangleIndices().size();
    return {index, count};
}

std::pair<size_t, size_t> BVH::FlattenRecursive(const BVHTreeNode& nodeA, const BVHTreeNode& nodeB)
{
    size_t indexA = nodes_.size();
    size_t indexB = nodes_.size() + 1;

    nodes_.emplace_back();
    nodes_.emplace_back();

    nodes_[indexA].SetAABB(nodeA.GetAABB());
    nodes_[indexB].SetAABB(nodeB.GetAABB());

    if (nodeA.IsLeaf())
    {
        auto [index, count] = AppendIndices(nodeA);
        nodes_[indexA].SetLeaf(index, count);
    }
    else
    {
        auto [childA, childB] = nodeA.GetChildren();
        auto [childAIndex, childBIndex] = FlattenRecursive(*childA, *childB);
        nodes_[indexA].SetInnerNode(childAIndex, childBIndex);
    }

    if (nodeB.IsLeaf())
    {
        auto [index, count] = AppendIndices(nodeB);
        nodes_[indexB].SetLeaf(index, count);
    }
    else
    {
        auto [childA, childB] = nodeB.GetChildren();
        auto [childAIndex, childBIndex] = FlattenRecursive(*childA, *childB);
        nodes_[indexB].SetInnerNode(childAIndex, childBIndex);
    }

    return {indexA, indexB};
}

void BVH::Flatten(const BVHTreeNode& root)
{
    // Children of each node are placed next to each other in the final array, so we can just push
    // them in pairs. This helps cache locality a bit when traversing the BVH.
    nodes_.emplace_back();
    nodes_.emplace_back();

    auto [childA, childB] = root.GetChildren();
    auto [childAIndex, childBIndex] = FlattenRecursive(*childA, *childB);
    nodes_[0].SetAABB(root.GetAABB());
    nodes_[0].SetInnerNode(childAIndex, childBIndex);
}

BVH::BVH(const Mesh& mesh, unsigned maxTrianglesPerNode, unsigned maxDepth)
{
    BVHTree tree(mesh.GetPositions(), maxTrianglesPerNode, maxDepth);
    Flatten(*tree.GetRoot());

    spdlog::debug("Generated BVH for mesh {}, nodes: {}, depth: {}, triangles: {}, indices: {}",
        mesh.GetName(), nodes_.size(), tree.GetRoot()->GetMaxDepthRecursive(),
        tree.GetTriangleCount(), indices_.size());
}
