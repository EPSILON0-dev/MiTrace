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
    std::vector<glm::vec3> positions_;  // REQUIRED
    std::vector<glm::vec3> normals_;    // REQUIRED
    std::vector<glm::vec4> tangents_;   // OPTIONAL
    std::vector<glm::vec2> texCoord0_;  // OPTIONAL (Main texture)
    std::vector<glm::vec2> texCoord1_;  // OPTIONAL (Reserved for future use, e.g. lightmaps)
    std::vector<glm::vec4> color0_;     // OPTIONAL
    std::vector<uint32_t> indices_;     // OPTIONAL

   public:
    Mesh() {}

    Mesh(const Loader::Mesh& mesh)
        : name_(mesh.name),
          positions_(mesh.positions),
          normals_(mesh.normals),
          tangents_(mesh.tangents),
          texCoord0_(mesh.texCoord0),
          texCoord1_(mesh.texCoord1),
          color0_(mesh.color0),
          indices_(mesh.indices)
    {
    }

   public:
    const auto& GetName() const noexcept { return name_; }
    const auto& GetPositions() const noexcept { return positions_; }
    const auto& GetNormals() const noexcept { return normals_; }
    const auto& GetTangents() const noexcept { return tangents_; }
    const auto& GetTexCoord0() const noexcept { return texCoord0_; }
    const auto& GetTexCoord1() const noexcept { return texCoord1_; }
    const auto& GetColor0() const noexcept { return color0_; }
    const auto& GetIndices() const noexcept { return indices_; }

    auto GetTriangleCount() const noexcept;

   public:
    std::pair<glm::vec3, glm::vec3> CalculateAABB() const noexcept;
};

}  // namespace Scene