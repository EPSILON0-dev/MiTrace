#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace Tracer::BRDF
{

static glm::vec3 FresnelSchlick(float cosTheta, const glm::vec3& f0) noexcept
{
    cosTheta = glm::clamp(cosTheta, 0.0f, 1.0f);
    return f0 + (glm::vec3(1.0f) - f0) * glm::pow(glm::vec3(1.0f - cosTheta), glm::vec3(5.0f));
}

// Note causes darkness for high metallness
static glm::vec3 BaseReflectivity(const glm::vec3& baseColor, float metallic) noexcept
{
    metallic = glm::clamp(metallic, 0.0f, 1.0f);
    return glm::mix(glm::vec3(0.04f), baseColor, metallic);
}

static float DistributionGGX(float gotNH, float roughness) noexcept
{
    gotNH = glm::clamp(gotNH, 0.0f, 1.0f);
    float a = glm::max(roughness * roughness, 1e-4f);
    float a2 = a * a;
    float dotNH2 = gotNH * gotNH;

    float denom = (dotNH2 * (a2 - 1.0f) + 1.0f);
    denom = glm::max(glm::pi<decltype(denom)>() * denom * denom, 1e-12f);

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

static float FonAlbedo(float mu, float r) noexcept
{
    mu = glm::clamp(mu, 0.0f, 1.0f);
    const float c = 1.0f - mu;
    const float g = c * (0.0571085289f + c * (0.491881867f + c * (-0.332181442f + c * 0.0714429953f)));
    return glm::min((1.0f + r * g) / (1.0f + 0.28791033f * r), 1.0f);
}

static glm::vec3 DiffuseEON(const glm::vec3& rho, float r, float dotNL, float dotNV, float dotLV) noexcept
{
    r = glm::clamp(r, 0.0f, 1.0f);
    const float s = dotLV - dotNL * dotNV;
    const float t = (s > 0.0f) ? glm::max(glm::max(dotNL, dotNV), s) : 1.0f;
    const float af = 1.0f / (1.0f + 0.28791033f * r);
    const glm::vec3 single = (rho / glm::pi<float>()) * af * (1.0f + r * s / glm::max(t, 1e-4f));
    const float eFo = FonAlbedo(dotNV, r);
    const float eFi = FonAlbedo(dotNL, r);
    const float avg = af * (1.0f + 0.07248355f * r);
    const glm::vec3 denom = glm::max(glm::vec3(1.0f) - rho * (1.0f - avg), glm::vec3(1e-4f));
    const glm::vec3 rms = (rho * rho) * avg / denom;
    return single + (rms / glm::pi<float>()) * ((1.0f - eFo) * (1.0f - eFi) / glm::max(1.0f - avg, 1e-7f));
}

inline glm::vec3 EvaluateBRDF(const glm::vec3& L, const glm::vec3& V, const glm::vec3& N,
    const glm::vec3& baseColor, float roughness, float metallic) noexcept
{
    roughness = glm::clamp(roughness, 0.01f, 1.0f);

    float dotNL = glm::max(glm::dot(N, L), 0.0f);
    float dotNV = glm::max(glm::dot(N, V), 0.0f);
    if (dotNL <= 0.0f || dotNV <= 0.0f) return glm::vec3(0.0f);

    glm::vec3 hl = V + L;
    if (!(glm::dot(hl, hl) > 1e-12f)) return glm::vec3(0.0f);
    glm::vec3 h = hl / std::sqrt(glm::dot(hl, hl));
    float dotNH = glm::max(glm::dot(N, h), 0.0f);
    float dotHV = glm::max(glm::dot(h, V), 0.0f);

    const glm::vec3 f0 = BaseReflectivity(baseColor, metallic);
    glm::vec3 f = FresnelSchlick(dotHV, f0);
    float d = DistributionGGX(dotNH, roughness);
    float g = GeometrySmith(dotNV, dotNL, roughness);

    glm::vec3 ks = f;
    glm::vec3 kd = (glm::vec3(1.0f) - ks) * (1.0f - metallic);

    glm::vec3 diffuse = kd * DiffuseEON(baseColor, roughness, dotNL, dotNV, glm::dot(L, V));
    glm::vec3 specular = (d * g * f) / glm::max(4.0f * dotNV * dotNL, 0.001f);

    return glm::max(diffuse + specular, glm::vec3(0.0f));
}

inline float DiffusePdf(const glm::vec3& L, const glm::vec3& N) noexcept
{
    return glm::max(glm::dot(N, L), 0.0f) / glm::pi<float>();
}

inline float SpecularPdf(
    const glm::vec3& L, const glm::vec3& V, const glm::vec3& N, float roughness) noexcept
{
    const float dotNL = glm::max(glm::dot(N, L), 0.0f);
    const float dotNV = glm::max(glm::dot(N, V), 0.0f);
    if (dotNL <= 0.0f || dotNV <= 0.0f) return 0.0f;
    roughness = glm::min(roughness, 1.0f);

    const glm::vec3 hl = V + L;
    if (!(glm::dot(hl, hl) > 1e-12f)) return 0.0f;
    const glm::vec3 h = hl / std::sqrt(glm::dot(hl, hl));
    const float dotNH = glm::max(glm::dot(N, h), 0.0f);

    return DistributionGGX(dotNH, roughness) * GeometrySchlickGGX(dotNV, roughness) /
           glm::max(4.0f * dotNV, 1e-6f);
}

}  // namespace BasicBackend::BRDF
