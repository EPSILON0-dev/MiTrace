#include "MaterialGLTF.hpp"

#include <glm/fwd.hpp>

void MaterialGLTF::Reflect(
    const GeometryInfo& geometryInfo, glm::vec3& direction, glm::vec3& energy) const noexcept
{
    direction = glm::reflect(direction, geometryInfo.normal);
    energy *= baseColorTexture_.IsValid()
                  ? glm::vec3(baseColorTexture_.Sample(geometryInfo.uv)) / 255.0f
                  : glm::vec3(baseColorFactor_);
}
