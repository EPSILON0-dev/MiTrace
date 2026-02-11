#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace BRDF
{

float FresnelSchlick(float cosTheta, float f0) noexcept
{
    float f = glm::clamp(f0 * (1.0f - 0.04f) + 0.04f, 0.0f, 1.0f);
    return f + (1.0f - f) * glm::pow(1.0f - cosTheta, 5.0f);
}

float DistributionGGX(float gotNH, float roughness) noexcept
{
    float a = roughness * roughness;
    float a2 = a * a;
    float dotNH2 = gotNH * gotNH;

    float denom = (dotNH2 * (a2 - 1.0f) + 1.0f);
    denom = glm::pi<decltype(denom)>() * denom * denom;

    return a2 / denom;
}

float GeometrySchlickGGX(float dotNV, float roughness) noexcept
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float denom = dotNV * (1.0f - k) + k;
    return dotNV / denom;
}

float GeometrySmith(float dotNV, float dotNL, float roughness) noexcept
{
    float ggx1 = GeometrySchlickGGX(dotNV, roughness);
    float ggx2 = GeometrySchlickGGX(dotNL, roughness);
    return ggx1 * ggx2;
}

/*
float CookTorrance(float NdotL, float NdotV, float NdotH, float roughness, float metalness) noexcept
{
    float d = DistributionGGX(NdotH, roughness);
    float g = GeometrySmith(NdotV, NdotL, roughness);
    float f = FresnelSchlick(NdotL, metalness);

    float numerator = d * g * f;
    float denominator = 4.0f * NdotV * NdotL + 0.001f;  // Prevent division by zero

    return numerator / denominator;
}
*/

float BRDF(const glm::vec3& L, const glm::vec3& V, const glm::vec3& N, float roughness,
    float metallic) noexcept
{
    // Roughness hack
    // roughness = glm::max(roughness, 0.05f);

    glm::vec3 h = glm::normalize(V + L);

    float dotNL = glm::max(glm::dot(N, L), 0.0f);
    float dotNV = glm::max(glm::dot(N, V), 0.0f);
    float dotNH = glm::max(glm::dot(N, h), 0.0f);
    float dotHV = glm::max(glm::dot(h, V), 0.0f);

    float f = FresnelSchlick(dotHV, metallic);
    float d = DistributionGGX(dotNH, roughness);
    float g = GeometrySmith(dotNV, dotNL, roughness);

    float ks = f;
    float kd = (1.0f - ks) * (1.0f - metallic);

    float specular = (d * g * f) / glm::max(4.0f * dotNV * dotNL, 0.001f);
    float diffuse = dotNL;

    float brdf = glm::max(kd * diffuse + ks * specular, 0.0f);
    return brdf;
}

}  // namespace BRDF
