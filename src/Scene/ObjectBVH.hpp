#pragma once

#include <cstdint>
#include <vector>

#include "BVH.hpp"

namespace Scene
{

class MeshInstance;

class ObjectBVH
{
   private:
    struct ObjectInstance
    {
        glm::vec3 min;
        glm::vec3 max;
        glm::vec3 centroid;
        uint32_t instanceIndex;
    };

    std::vector<BVHNode> nodes_;
    std::vector<uint32_t> instanceIndices_;

    uint32_t BuildRecursive(std::vector<ObjectInstance> items, uint32_t depth);

   public:
    ObjectBVH() = default;
    explicit ObjectBVH(const std::vector<MeshInstance>& instances);

    bool IsEmpty() const noexcept { return nodes_.empty(); }
    const std::vector<BVHNode>& GetNodes() const noexcept { return nodes_; }
    const std::vector<uint32_t>& GetInstanceIndices() const noexcept { return instanceIndices_; }
};

}  // namespace Scene
