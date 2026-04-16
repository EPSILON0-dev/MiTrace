#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace BasicBackend::BRDF
{

static glm::vec3 FresnelSchlick(float cosTheta, const glm::vec3& f0) noexcept
{
    cosTheta = glm::clamp(cosTheta, 0.0f, 1.0f);
    return f0 + (glm::vec3(1.0f) - f0) * glm::pow(glm::vec3(1.0f - cosTheta), glm::vec3(5.0f));
}

static glm::vec3 BaseReflectivity(const glm::vec3& baseColor, float metallic) noexcept
{
    metallic = glm::clamp(metallic, 0.0f, 1.0f);
    return glm::mix(glm::vec3(0.04f), baseColor, metallic);
}

static float DistributionGGX(float gotNH, float roughness) noexcept
{
    gotNH = glm::clamp(gotNH, 0.0f, 1.0f);
    float a = roughness * roughness;
    float a2 = a * a;
    float dotNH2 = gotNH * gotNH;

    float denom = (dotNH2 * (a2 - 1.0f) + 1.0f);
    denom = glm::pi<decltype(denom)>() * denom * denom;

    return a2 / denom;
}

static float GeometrySchlickGGX(float dotNV, float roughness) noexcept
{
    dotNV = glm::clamp(dotNV, 0.0f, 1.0f);
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float denom = dotNV * (1.0f - k) + k;
    return dotNV / denom;
}

static float GeometrySmith(float dotNV, float dotNL, float roughness) noexcept
{
    float ggx1 = GeometrySchlickGGX(dotNV, roughness);
    float ggx2 = GeometrySchlickGGX(dotNL, roughness);
    return ggx1 * ggx2;
}

static glm::vec3 EvaluateBRDF(const glm::vec3& L, const glm::vec3& V, const glm::vec3& N,
    const glm::vec3& baseColor, float roughness, float metallic) noexcept
{
    roughness = glm::max(roughness, 0.05f);

    float dotNL = glm::max(glm::dot(N, L), 0.0f);
    float dotNV = glm::max(glm::dot(N, V), 0.0f);
    if (dotNL <= 0.0f || dotNV <= 0.0f) return glm::vec3(0.0f);

    glm::vec3 h = glm::normalize(V + L);
    float dotNH = glm::max(glm::dot(N, h), 0.0f);
    float dotHV = glm::max(glm::dot(h, V), 0.0f);

    const glm::vec3 f0 = BaseReflectivity(baseColor, metallic);
    glm::vec3 f = FresnelSchlick(dotHV, f0);
    float d = DistributionGGX(dotNH, roughness);
    float g = GeometrySmith(dotNV, dotNL, roughness);

    glm::vec3 ks = f;
    glm::vec3 kd = (glm::vec3(1.0f) - ks) * (1.0f - metallic);

    glm::vec3 diffuse = kd * baseColor / glm::pi<float>();
    glm::vec3 specular = (d * g * f) / glm::max(4.0f * dotNV * dotNL, 0.001f);

    return glm::max(diffuse + specular, glm::vec3(0.0f));
}

static float DiffusePdf(const glm::vec3& L, const glm::vec3& N) noexcept
{
    return glm::max(glm::dot(N, L), 0.0f) / glm::pi<float>();
}

static float SpecularPdf(
    const glm::vec3& L, const glm::vec3& V, const glm::vec3& N, float roughness) noexcept
{
    const float dotNL = glm::max(glm::dot(N, L), 0.0f);
    const float dotNV = glm::max(glm::dot(N, V), 0.0f);
    if (dotNL <= 0.0f || dotNV <= 0.0f) return 0.0f;

    const glm::vec3 h = glm::normalize(V + L);
    const float dotNH = glm::max(glm::dot(N, h), 0.0f);
    const float dotVH = glm::max(glm::dot(V, h), 0.0f);
    if (dotVH <= 0.0f) return 0.0f;

    return DistributionGGX(dotNH, glm::max(roughness, 0.05f)) * dotNH / (4.0f * dotVH);
}

}  // namespace BasicBackend::BRDF
