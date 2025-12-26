// Most of the eqations in this file were sourced from https://learnopengl.com/PBR/Theory
#include "MaterialGLTF.hpp"

#include <glm/gtc/constants.hpp>
#include <random>

/*
// TODO move to dedicated PBR utility file
// Constants for PBR calculations
constexpr float fresnelPower = 5.0f;
constexpr float fresnelMinimal = 0.04f;

static glm::vec3 ComputeHalfWayVector(const glm::vec3& viewVec, const glm::vec3& lightVec) noexcept
{
    return glm::normalize(viewVec + lightVec);
}

// Fresnel Schlick approximation
static float ComputeFresnelSchlick(float HdotV, float F0) noexcept
{
    return F0 + (1.0f - F0) * glm::pow(1.0f - HdotV, fresnelPower);
}

// GGX / Trowbridge-Reitz normal distribution function
static float ComputeNormalDistributionGGXTR(float NdotH, float alpha) noexcept
{
    const float a2 = alpha * alpha;
    const float denom = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;
    return a2 / (glm::pi<float>() * denom * denom);
}

// Geometry function using Schlick-GGX approximation
static float ComputeGeometrySchlickGGX(float NdotV, float k) noexcept
{
    return NdotV / (NdotV * (1.0f - k) + k);
}

static float ComputeGeometrySmith(float NdotV, float NdotL, float k) noexcept
{
    const float ggxV = ComputeGeometrySchlickGGX(NdotV, k);
    const float ggxL = ComputeGeometrySchlickGGX(NdotL, k);
    return ggxV * ggxL;
}

void MaterialGLTF::Reflect(const RayHitGeometryInfo& geomInfo, glm::vec3& viewVec,
    glm::vec3& energyMultiplier, std::mt19937& rng) const noexcept
{
    // We're doing math here, math doesn't need readable identifier names
    // NOLINTBEGIN(readability-identifier-naming)

    // Fetch material properties
    const auto metallic = GetMetallic(geomInfo.TexCoord0);

    // Compute the fresnel term to determine reflection vs diffuse
    const auto effectiveMetallic = metallic * (1.0f - fresnelMinimal) + fresnelMinimal;
    const auto NdotV = glm::max(glm::dot(geomInfo.Normal, glm::normalize(viewVec)), 0.0f);
    const auto ks = ComputeFresnelSchlick(NdotV, effectiveMetallic);

    // Decide between reflection and diffuse based on fresnel
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    const float randomSample = dist(rng);
    if (randomSample < ks)
    {
        // Reflect the view vector around the normal for perfect specular reflection
        viewVec = glm::reflect(-viewVec, geomInfo.Normal);
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
    energyMultiplier *= (1.0f - effectiveMetallic);

    // NOLINTEND(readability-identifier-naming)
}

glm::vec3 MaterialGLTF::ComputeLightContribution(const RayHitGeometryInfo& geomInfo,
    const glm::vec3& viewVec, const glm::vec3& lightVec, const glm::vec3& lightColor) const noexcept
{
    // We're doing math here, math doesn't need readable identifier names
    // NOLINTBEGIN(readability-identifier-naming)

    // Fetch material properties
    glm::vec3 albedo = GetBaseColor(geomInfo.TexCoord0);
    float metallic = GetMetallic(geomInfo.TexCoord0);
    float roughness = GetRoughness(geomInfo.TexCoord0);
    // glm::vec3 normalMap = GetNormal(geomInfo.TexCoord0);
    // TODO occlusion

    // Compute necessary vectors and dot products
    const auto N = glm::normalize(geomInfo.Normal); // TODO transform normal into the world space
    const auto V = glm::normalize(viewVec);
    const auto L = glm::normalize(lightVec);
    const auto H = ComputeHalfWayVector(V, L);
    const auto NdotL = glm::max(glm::dot(N, L), 0.0f);
    const auto NdotV = glm::max(glm::dot(N, V), 0.0f);
    const auto NdotH = glm::max(glm::dot(N, H), 0.0f);
    const auto HdotV = glm::max(glm::dot(H, V), 0.0f);

    // Compute the fresnel reflectance at normal incidence
    const auto effectiveMetallic = metallic * (1.0f - fresnelMinimal) + fresnelMinimal;
    const auto ks = ComputeFresnelSchlick(HdotV, effectiveMetallic);
    const auto kd = 1.0f - ks;

    // Conpute BRDF terms
    const auto alpha = roughness * roughness;
    const auto k_G = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
    const auto D = ComputeNormalDistributionGGXTR(NdotH, alpha);
    const auto G = ComputeGeometrySmith(NdotV, NdotL, k_G);
    const auto f_spec_num = D * G * ks;
    const auto f_spec_denom = 4.0f * NdotV * NdotL + 0.001f;
    const auto f_spec = f_spec_num / f_spec_denom;
    const auto f_diff = (kd * albedo) / glm::pi<float>();
    const auto BRDF = kd * f_diff + ks * f_spec;

    // Final light contribution
    return (BRDF * lightColor * NdotL);

    // NOLINTEND(readability-identifier-naming)
}
*/

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
    return (albedo * lightColor * dotNL) * (1.0f - metallic) / (lightDist * lightDist * glm::pi<float>());
}
