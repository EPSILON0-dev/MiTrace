/**
 * @file Material.hpp
 *
 * Material representation including textures (with image references) and
 * factors for rendering.
 */
#pragma once

#include <glm/fwd.hpp>
#include <string>

#include "Trace/Texture.hpp"

// TODO emission strength, ior and transmission extensions

class Material
{
   public:
    enum class TransparencyMode : uint8_t
    {
        OPAQUE,
        MASK,
        BLEND
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
    float alphaCutoff_;
    TransparencyMode transparencyMode_;
    bool doubleSided_;

   public:
    Material()
        : name_("empty"),
          baseColorTexture_(nullptr),
          metallicRoughnessTexture_(nullptr),
          normalTexture_(nullptr),
          occlusionTexture_(nullptr),
          emissiveTexture_(nullptr),
          baseColorFactor_(1.0f),
          emissiveFactor_(0.0f),
          normalScale_(1.0f),
          metallicFactor_(1.0f),
          roughnessFactor_(1.0f),
          occlusionStrength_(1.0f)
    {
    }

   public:
    void SetName(const auto& name) noexcept { name_ = name; }
    void SetBaseColorTexture(const auto& baseColorTexture) noexcept
    {
        baseColorTexture_ = baseColorTexture;
    }
    void SetMetallicRoughnessTexture(const auto& metallicRoughnessTexture) noexcept
    {
        metallicRoughnessTexture_ = metallicRoughnessTexture;
    }
    void SetNormalTexture(const auto& normalTexture) noexcept { normalTexture_ = normalTexture; }
    void SetOcclusionTexture(const auto& occlusionTexture) noexcept
    {
        occlusionTexture_ = occlusionTexture;
    }
    void SetEmissiveTexture(const auto& emissiveTexture) noexcept
    {
        emissiveTexture_ = emissiveTexture;
    }
    void SetBaseColorFactor(const auto& baseColorFactor) noexcept
    {
        baseColorFactor_ = baseColorFactor;
    }
    void SetEmissiveFactor(const auto& emissiveFactor) noexcept
    {
        emissiveFactor_ = emissiveFactor;
    }
    void SetNormalScale(const auto& normalScale) noexcept { normalScale_ = normalScale; }
    void SetMetallicFactor(const auto& metallicFactor) noexcept
    {
        metallicFactor_ = metallicFactor;
    }
    void SetRoughnessFactor(const auto& roughnessFactor) noexcept
    {
        roughnessFactor_ = roughnessFactor;
    }
    void SetOcclusionStrength(const auto& occlusionStrength) noexcept
    {
        occlusionStrength_ = occlusionStrength;
    }
    void SetAlphaCutoff(const auto& alphaCutoff) noexcept { alphaCutoff_ = alphaCutoff; }
    void SetTransparencyMode(const auto& transparencyMode) noexcept
    {
        transparencyMode_ = transparencyMode;
    }
    void SetDoubleSided(const auto& doubleSided) noexcept { doubleSided_ = doubleSided; }

   public:
    const auto& GetName() const noexcept { return name_; }

    glm::vec4 GetBaseColor(const glm::vec2& texCoord) const noexcept
    {
        return baseColorTexture_.IsValid()
                   ? glm::vec4(baseColorTexture_.Sample(texCoord)) / 255.0f * baseColorFactor_
                   : glm::vec4(baseColorFactor_);
    }

    float GetMetallic(const glm::vec2& texCoord) const noexcept
    {
        return metallicRoughnessTexture_.IsValid()
                   ? metallicRoughnessTexture_.Sample(texCoord).r / 255.0f * metallicFactor_
                   : metallicFactor_;
    }

    float GetRoughness(const glm::vec2& texCoord) const noexcept
    {
        return metallicRoughnessTexture_.IsValid()
                   ? metallicRoughnessTexture_.Sample(texCoord).g / 255.0f * roughnessFactor_
                   : roughnessFactor_;
    }

    glm::vec3 GetNormal(const glm::vec2& texCoord) const noexcept
    {
        return normalTexture_.IsValid()
                   ? glm::vec3(normalTexture_.Sample(texCoord)) / 255.0f * normalScale_
                   : glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::vec3 GetEmissive(const glm::vec2& texCoord) const noexcept
    {
        return emissiveTexture_.IsValid()
                   ? glm::vec3(emissiveTexture_.Sample(texCoord)) / 255.0f * emissiveFactor_
                   : emissiveFactor_;
    }

    glm::vec3 GetOcclusion(const glm::vec2& texCoord) const noexcept
    {
        return occlusionTexture_.IsValid()
                   ? glm::vec3(occlusionTexture_.Sample(texCoord)) / 255.0f * occlusionStrength_
                   : glm::vec3(1.0f);
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
