#include "MaterialGLTF.hpp"

#include <glm/fwd.hpp>

void MaterialGLTF::Reflect(
    const RayHitGeometryInfo& geomInfo, glm::vec3& direction, glm::vec3& energy) const noexcept
{
    direction = glm::reflect(direction, geomInfo.Normal);
    energy *= baseColorTexture_.IsValid()
                  ? glm::vec3(baseColorTexture_.Sample(geomInfo.TexCoord0)) / 255.0f
                  : glm::vec3(baseColorFactor_);
}
