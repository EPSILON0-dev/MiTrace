#pragma once

#include <glm/fwd.hpp>
#include <string>

#include "Texture.hpp"

namespace Scene
{

class Material
{
   public:
    enum class TransparencyMode : uint8_t
    {
        Opaque,
        Mask,
        Blend
    };

   public:
    struct MaterialPoint
    {
        glm::vec4 baseColor;
        float metallic;
        float roughness;
        glm::vec3 normal;
        glm::vec3 emissive;
        glm::vec3 occlusion;
    };

   private:
    std::string name_;
    Texture baseColorTexture_;
    Texture metallicRoughnessTexture_;
    Texture normalTexture_;
    Texture occlusionTexture_;
    Texture emissiveTexture_;
    glm::vec4 baseColorFactor_;
    glm::vec3 emissiveFactor_;
    float normalScale_;
    float metallicFactor_;
    float roughnessFactor_;
    float occlusionStrength_;

    // Currrently unused
    // float alphaCutoff_;
    // TransparencyMode transparencyMode_;
    // bool doubleSided_;

   public:
    Material()
        : name_("empty"),
          baseColorFactor_(1.0f),
          emissiveFactor_(0.0f),
          normalScale_(1.0f),
          metallicFactor_(1.0f),
          roughnessFactor_(1.0f),
          occlusionStrength_(1.0f)
    {
    }

    Material(const Loader::Material& material)
        : name_(material.name),
          baseColorTexture_(material.baseColorTexture),
          metallicRoughnessTexture_(material.metallicRoughnessTexture),
          normalTexture_(material.normalTexture),
          occlusionTexture_(material.occlusionTexture),
          emissiveTexture_(material.emissiveTexture),
          baseColorFactor_(material.baseColorFactor),
          emissiveFactor_(material.emissiveFactor),
          normalScale_(material.normalScale),
          metallicFactor_(material.metallicFactor),
          roughnessFactor_(material.roughnessFactor),
          occlusionStrength_(material.occlusionStrength)
    {
    }

   public:
    const auto& GetName() const noexcept { return name_; }

    glm::vec4 GetBaseColor(const glm::vec2& texCoord) const noexcept
    {
        return baseColorTexture_.IsValid()
                   ? glm::vec4(baseColorTexture_.Sample(texCoord)) * baseColorFactor_
                   : glm::vec4(baseColorFactor_);
    }

    glm::vec3 GetOcclusion(const glm::vec2& texCoord) const noexcept
    {
        return occlusionTexture_.IsValid()
                   ? glm::vec3(occlusionTexture_.Sample(texCoord).r) * occlusionStrength_
                   : glm::vec3(1.0f);
    }

    float GetRoughness(const glm::vec2& texCoord) const noexcept
    {
        return metallicRoughnessTexture_.IsValid()
                   ? metallicRoughnessTexture_.Sample(texCoord).g * roughnessFactor_
                   : roughnessFactor_;
    }

    float GetMetallic(const glm::vec2& texCoord) const noexcept
    {
        return metallicRoughnessTexture_.IsValid()
                   ? metallicRoughnessTexture_.Sample(texCoord).b * metallicFactor_
                   : metallicFactor_;
    }

    glm::vec3 GetNormal(const glm::vec2& texCoord) const noexcept
    {
        return normalTexture_.IsValid()
                   ? glm::normalize(glm::vec3(normalTexture_.Sample(texCoord)) * normalScale_ * 2.0f - 1.0f)
                   : glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::vec3 GetEmissive(const glm::vec2& texCoord) const noexcept
    {
        return emissiveTexture_.IsValid()
                   ? glm::vec3(emissiveTexture_.Sample(texCoord)) * emissiveFactor_
                   : emissiveFactor_;
    }

    MaterialPoint SampleMaterial(const glm::vec2& texCoord) const noexcept
    {
        MaterialPoint point;
        point.baseColor = GetBaseColor(texCoord);
        point.metallic = GetMetallic(texCoord);
        point.roughness = GetRoughness(texCoord);
        point.normal = GetNormal(texCoord);
        point.emissive = GetEmissive(texCoord);
        point.occlusion = GetOcclusion(texCoord);
        return point;
    }
};

}  // namespace Scene
