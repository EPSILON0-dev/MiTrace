#pragma once

#include <glm/glm.hpp>
#include <string>

#include "Loader/Types.hpp"

namespace Scene
{

class Mesh
{
   private:
    std::string name_;
    std::vector<glm::vec3> positions_;
    std::vector<glm::vec3> normals_;
    std::vector<glm::vec4> tangents_;
    std::vector<glm::vec2> texCoord0_;
    std::vector<glm::vec2> texCoord1_;
    std::vector<glm::vec4> color0_;
    
    // Indices are dropped when converting from the Loader Mesh
    // std::vector<uint32_t> indices_;

   public:
    Mesh() {}

    Mesh(const Loader::Mesh& mesh);

   public:
    const auto& GetName() const noexcept { return name_; }
    const auto& GetPositions() const noexcept { return positions_; }
    const auto& GetNormals() const noexcept { return normals_; }
    const auto& GetTangents() const noexcept { return tangents_; }
    const auto& GetTexCoord0() const noexcept { return texCoord0_; }
    const auto& GetTexCoord1() const noexcept { return texCoord1_; }
    const auto& GetColor0() const noexcept { return color0_; }

    auto GetTriangleCount() const noexcept { return positions_.size() / 3; }

   public:
    std::pair<glm::vec3, glm::vec3> CalculateAABB() const noexcept;
};

}  // namespace Scene
