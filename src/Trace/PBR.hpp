// Most of the eqations in this file were sourced from https://learnopengl.com/PBR/Theory
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// NOLINTBEGIN(readability-identifier-naming)

namespace PBR
{

// Constants for PBR calculations
constexpr float FRESNEL_POWER = 5.0f;
constexpr float FRESNEL_MINIMAL = 0.04f;

inline glm::vec3 HalfVec(const glm::vec3& viewVec, const glm::vec3& lightVec) noexcept
{
    return glm::normalize(viewVec + lightVec);
}

// Fresnel Schlick approximation
inline float F_FS(float HdotV, float F0) noexcept
{
    return F0 + (1.0f - F0) * glm::pow(1.0f - HdotV, FRESNEL_POWER);
}

// GGX / Trowbridge-Reitz normal distribution function
inline float D_GGX_TR(float NdotH, float alpha) noexcept
{
    const auto a2 = alpha * alpha;
    const auto denom = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;
    return a2 / (glm::pi<float>() * denom * denom);
}

// Geometry function using Schlick-GGX approximation
inline float G_SchlickGGX(float NdotV, float k) noexcept
{
    return NdotV / (NdotV * (1.0f - k) + k);
}

// Geometry function using Smith's method
inline float G_Smith(float NdotV, float NdotL, float k) noexcept
{
    const auto ggxV = G_SchlickGGX(NdotV, k);
    const auto ggxL = G_SchlickGGX(NdotL, k);
    return ggxV * ggxL;
}

// Cook-Torrance BRDF
inline float CookTorranceBRDF(
    float NdotL, float NdotV, float NdotH, float HdotV, float alpha, float F0) noexcept
{
    const auto kG = (alpha + 1.0f) * (alpha + 1.0f) / 8.0f;
    const auto D = D_GGX_TR(NdotH, alpha);
    const auto G = G_Smith(NdotV, NdotL, kG);
    const auto F = F_FS(HdotV, F0);

    const auto numerator = D * G * F;
    const auto denominator = 4.0f * NdotV * NdotL + 0.001f;  // Prevent division by zero
    return numerator / denominator;
}

// Lambertian diffuse reflection

};  // namespace PBR

// NOLINTEND(readability-identifier-naming)
