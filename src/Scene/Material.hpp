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
        Blend,
    };

   public:
    struct MaterialPoint
    {
        glm::vec4 baseColor;
        float metallic;
        float roughness;
        glm::vec3 normal;
        glm::vec3 emission;
        float occlusion;
        float alpha;
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

    float alphaCutoff_;
    TransparencyMode transparencyMode_;
    bool doubleSided_;

   public:
    Material()
        : name_("empty"),
          baseColorFactor_(1.0f),
          emissiveFactor_(0.0f),
          normalScale_(1.0f),
          metallicFactor_(1.0f),
          roughnessFactor_(1.0f),
          occlusionStrength_(1.0f),
          alphaCutoff_(0.5f),
          transparencyMode_(TransparencyMode::Opaque),
          doubleSided_(false)
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
          occlusionStrength_(material.occlusionStrength),
          alphaCutoff_(material.alphaCutoff),
          transparencyMode_(TransparencyMode::Opaque),
          doubleSided_(material.doubleSided)
    {
        switch (material.transparencyMode)
        {
            case Loader::TransparencyMode::Opaque:
                transparencyMode_ = TransparencyMode::Opaque;
                break;
            case Loader::TransparencyMode::Mask:
                transparencyMode_ = TransparencyMode::Mask;
                break;
            case Loader::TransparencyMode::Blend:
                transparencyMode_ = TransparencyMode::Blend;
                break;
        }
    }

   public:
    const auto& GetName() const noexcept { return name_; }
    TransparencyMode GetTransparencyMode() const noexcept { return transparencyMode_; }
    float GetAlphaCutoff() const noexcept { return alphaCutoff_; }
    bool IsDoubleSided() const noexcept { return doubleSided_; }

    float GetAlpha(const glm::vec2& texCoord) const noexcept
    {
        return GetBaseColor(texCoord).a;
    }

    glm::vec4 GetBaseColor(const glm::vec2& texCoord) const noexcept
    {
        return baseColorTexture_.IsValid()
                   ? glm::vec4(baseColorTexture_.Sample(texCoord)) * baseColorFactor_
                   : glm::vec4(baseColorFactor_);
    }

    float GetOcclusion(const glm::vec2& texCoord) const noexcept
    {
        return occlusionTexture_.IsValid()
                   ? occlusionTexture_.Sample(texCoord).r * occlusionStrength_
                   : 1.0f;
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
                   ? glm::normalize(
                         glm::vec3(normalTexture_.Sample(texCoord)) * normalScale_ * 2.0f - 1.0f)
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
        point.emission = GetEmissive(texCoord);
        point.occlusion = GetOcclusion(texCoord);
        point.alpha = glm::clamp(point.baseColor.a, 0.0f, 1.0f);
        return point;
    }
};

}  // namespace Scene
