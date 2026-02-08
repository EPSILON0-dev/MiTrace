#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Scene
{

class BVHTreeNode
{
    using AABB = std::pair<glm::vec3, glm::vec3>;
    using BVHNodePtr = std::unique_ptr<BVHTreeNode>;
    using BVHNodeChildren = std::pair<BVHNodePtr, BVHNodePtr>;
    using BVHNodeChildrenRawPtr = std::pair<const BVHTreeNode*, const BVHTreeNode*>;
    using SplitIndices = std::pair<std::vector<uint32_t>, std::vector<uint32_t>>;

    enum class SplitAxis
    {
        X,
        Y,
        Z
    };

   private:
    const std::vector<glm::vec3>& allTriangles_;
    std::vector<uint32_t> allTriangleIndices_;
    AABB aabb_;
    BVHNodeChildren children_;
    int depth_ = 0;
    bool isLeaf_ = true;

   private:
    void ComputeAABB();
    glm::vec3 FindCenterOfMass() const;
    SplitIndices SplitTriangles(SplitAxis axis, float splitPos) const;
    float ComputeSplitScore(SplitIndices) const;

   public:
    BVHTreeNode(const std::vector<glm::vec3>& allTriangles,
        const std::vector<uint32_t>& triangleIndices, int depth = 0);
    BVHTreeNode(const std::vector<glm::vec3>& allTriangles,
        const std::vector<uint32_t>& triangleIndices, const AABB& aabb, int depth = 0);

    bool IsLeaf() const { return isLeaf_; }
    AABB GetAABB() const { return aabb_; }
    const std::vector<uint32_t>& GetTriangleIndices() const { return allTriangleIndices_; }
    BVHNodeChildrenRawPtr GetChildren() const
    {
        return {children_.first.get(), children_.second.get()};
    }

    void Subdivide(int maxTrianglesPerLeaf = 8, int maxDepth = 32);
};

class BVHTree
{
   private:
    const std::vector<glm::vec3>& allTriangles_;
    std::unique_ptr<BVHTreeNode> root_;

   private:
    uint32_t GetMaxDepthRecursive(const BVHTreeNode* node) const;

   public:
    BVHTree(
        const std::vector<glm::vec3>& triangles, int maxTrianglesPerLeaf = 8, int maxDepth = 32);

    int GetTriangleCount() const { return allTriangles_.size() / 3; }
    const BVHTreeNode* GetRoot() const { return root_.get(); }
};

class alignas(32) BVHNode
{
   private:
    static constexpr uint32_t leafBit = 0x80000000;

    std::pair<glm::vec3, glm::vec3> aabb_;
    union NodeData
    {
        struct
        {
            uint32_t a;
            uint32_t b;
        } children;
        struct
        {
            uint32_t index;
            uint32_t count;
        } triangles;
    };

    NodeData nodeData_;

   public:
    BVHNode() = default;

    void SetAABB(const std::pair<glm::vec3, glm::vec3>& aabb) { aabb_ = aabb; }

    void SetInnerNode(uint32_t childA, uint32_t childB)
    {
        nodeData_.children.a = childA;
        nodeData_.children.b = childB;
    }

    void SetLeaf(uint32_t triangleIndex, uint32_t triangleCount)
    {
        nodeData_.triangles.index = triangleIndex | leafBit;
        nodeData_.triangles.count = triangleCount;
    }

    inline const auto& GetAABB() const noexcept { return aabb_; }
    inline auto IsLeaf() const noexcept { return (nodeData_.children.a & leafBit) != 0; }
    inline auto GetChildA() const noexcept { return nodeData_.children.a; }
    inline auto GetChildB() const noexcept { return nodeData_.children.b; }
    inline auto GetTriangleIndex() const noexcept { return nodeData_.triangles.index & ~leafBit; }
    inline auto GetTriangleCount() const noexcept { return nodeData_.triangles.count & ~leafBit; }
};

class Mesh;  // Forward declaration
class BVH
{
   private:
    std::vector<BVHNode> nodes_;
    std::vector<uint32_t> indices_;

   private:
    void FlattenRecursive(const BVHTreeNode& node);

   public:
    BVH() = default;
    BVH(const Scene::Mesh& mesh, int maxTrianglesPerNode = 8, int maxDepth = 32);

    inline const std::vector<BVHNode>& GetNodes() const noexcept { return nodes_; }
    inline const std::vector<uint32_t>& GetIndices() const noexcept { return indices_; }
};

}  // namespace Scene
