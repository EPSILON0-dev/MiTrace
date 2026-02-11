#include "RayHit.hpp"

#include "Scene/Mesh.hpp"

RayHitGeometryInfo::RayHitGeometryInfo(const RayHit& rayHit) noexcept : RayHit(rayHit)
{
    const auto& mesh = meshInstance->GetMesh();
    const auto& transform = meshInstance->GetTransform();

    auto interpolate = [&](const auto& dataArray, bool normalize = false)
    {
        const auto& v0 = dataArray[0];
        const auto& v1 = dataArray[1];
        const auto& v2 = dataArray[2];
        if (normalize)
        {
            return glm::normalize(
                (1.0f - baryCoord.x - baryCoord.y) * v0 + baryCoord.x * v1 + baryCoord.y * v2);
        }
        else
        {
            return (1.0f - baryCoord.x - baryCoord.y) * v0 + baryCoord.x * v1 + baryCoord.y * v2;
        }
    };

    const auto& triangle = mesh.GetTriangles()[triangleIndex];
    Normal = glm::normalize(transform * glm::vec4(interpolate(triangle.normal, true), 0.0f));
    Flags = mesh.GetFlags();

    if (Flags.HasTangent)
    {
        Tangent = glm::normalize(
            transform * glm::vec4(glm::vec3(interpolate(triangle.tangent, true)), 0.0f));
    }

    if (Flags.HasTexCoord0)
    {
        TexCoord0 = interpolate(triangle.texCoord0, false);
    }

    if (Flags.HasTexCoord1)
    {
        TexCoord1 = interpolate(triangle.texCoord1, false);
    }

    if (Flags.HasColor0)
    {
        Color0 = interpolate(triangle.color0, false);
    }
}
