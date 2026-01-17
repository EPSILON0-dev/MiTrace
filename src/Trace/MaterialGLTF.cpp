#include "MaterialGLTF.hpp"

#include <glm/gtc/constants.hpp>
#include <random>

void MaterialGLTF::Reflect(const RayHitGeometryInfo& geomInfo, glm::vec3& viewVec,
    glm::vec3& energyMultiplier, std::mt19937& rng) const noexcept
{
    const auto color = glm::vec3(GetBaseColor(geomInfo.TexCoord0));
    const auto metallic = GetMetallic(geomInfo.TexCoord0);

    // Decide between reflection and diffuse based on fresnel
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    const float randomSample = dist(rng);
    if (randomSample < metallic)
    {
        // Reflect the view vector around the normal for perfect specular reflection
        viewVec = glm::reflect(viewVec, geomInfo.Normal);
    }
    else
    {
        // Simplified diffuse reflection: cosine-weighted hemisphere sampling
        std::uniform_real_distribution<float> phi(0.0f, glm::two_pi<float>());
        std::uniform_real_distribution<float> theta(-glm::half_pi<float>(), glm::half_pi<float>());
        const float u1 = dist(rng);
        const float u2 = dist(rng);
        const float r = glm::sqrt(u1);
        const float thetaSample = 2.0f * glm::pi<float>() * u2;
        const float x = r * glm::cos(thetaSample);
        const float y = r * glm::sin(thetaSample);
        const float z = glm::sqrt(glm::max(0.0f, 1.0f - u1));
        const glm::vec3 sampleDir(x, y, z);
        viewVec = glm::dot(sampleDir, geomInfo.Normal) < 0.0f ? -sampleDir : sampleDir;
    }

    // TODO actually implement this
    energyMultiplier = color * glm::vec3(0.7f);
}

glm::vec3 MaterialGLTF::ComputeLightContribution(const RayHitGeometryInfo& geomInfo,
    const glm::vec3& viewVec, const glm::vec3& lightVec, const glm::vec3& lightColor) const noexcept
{
    (void)viewVec;

    const auto albedo = static_cast<glm::vec3>(GetBaseColor(geomInfo.TexCoord0));
    const auto metallic = GetMetallic(geomInfo.TexCoord0);
    const auto normal = geomInfo.Normal;
    const auto light = glm::normalize(lightVec);
    const auto lightDist = glm::length(lightVec);
    const auto dotNL = glm::max(glm::dot(normal, light), 0.0f);
    const auto attenuation = (lightDist * lightDist);
    const auto effectiveAttenuation = glm::max(attenuation, 1.0f);
    return (albedo * lightColor * dotNL) * (1.0f - metallic) / (effectiveAttenuation * glm::pi<float>());
}
