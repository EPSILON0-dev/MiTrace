// Most of the eqations in this file were sourced from https://learnopengl.com/PBR/Theory
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace PBR
{

inline glm::vec3 HalfVec(const glm::vec3& viewVec, const glm::vec3& lightVec) noexcept
{
    return glm::normalize(viewVec + lightVec);
}

// Fresnel Schlick approximation
constexpr float fresnelPower = 5.0f;
constexpr float fresnelMinimal = 0.04f;
inline float FresnelSchlick(float dotNV, float metallic) noexcept
{
    metallic = metallic * (1.0f - fresnelMinimal) + fresnelMinimal;
    return metallic + (1.0f - metallic) * glm::pow(1.0f - dotNV, fresnelPower);
}

};  // namespace PBR
