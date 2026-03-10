#include "RayHit.hpp"

#include "Scene/Mesh.hpp"

RayHitGeometryInfo::RayHitGeometryInfo(const RayHit& rayHit) noexcept
    : RayHit(rayHit), Flags{false, false, false, false}
{
    const auto& mesh = meshInstance->GetMesh();
    const auto& transform = meshInstance->GetTransform();

    auto interpolate = [&](const auto& dataArray, bool normalize = false)
    {
        const auto& v0 = dataArray[triangleIndex * 3 + 0];
        const auto& v1 = dataArray[triangleIndex * 3 + 1];
        const auto& v2 = dataArray[triangleIndex * 3 + 2];
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

    Normal = glm::normalize(transform * glm::vec4(interpolate(mesh.GetNormals(), true), 0.0f));

    /*
    if (!mesh.GetTangents().empty())
    {
        Tangent = glm::normalize(
            transform * glm::vec4(glm::vec3(interpolate(mesh.GetTangents(), true)), 0.0f));
        Flags.HasTangent = true;
    }
    */

    if (!mesh.GetTexCoord0().empty())
    {
        TexCoord0 = interpolate(mesh.GetTexCoord0(), false);
        Flags.HasTexCoord0 = true;
    }

    if (!mesh.GetTexCoord1().empty())
    {
        TexCoord1 = interpolate(mesh.GetTexCoord1(), false);
        Flags.HasTexCoord1 = true;
    }

    if (!mesh.GetColor0().empty())
    {
        Color0 = interpolate(mesh.GetColor0(), false);
        Flags.HasColor0 = true;
    }
}
