#pragma once

#include <optional>
#include <string>

#include "MaterialBase.hpp"
#include "Trace/Texture.hpp"

class MaterialGLTF : public MaterialBase
{
    friend class GLTF_Loader;

   public:
    enum class TransparencyMode : uint8_t
    {
        OPAQUE,
        MASK,
        BLEND
    };

   private:
    std::string name_;
    std::optional<Texture> baseColorTexture_;
    std::optional<Texture> metallicRoughnessTexture_;
    std::optional<Texture> normalTexture_;
    std::optional<Texture> occlusionTexture_;
    std::optional<Texture> emissiveTexture_;
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
    MaterialGLTF()
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

    virtual void Reflect(const GeometryInfo& geometryInfo, glm::vec3& direction,
        glm::vec3& energy) const noexcept override;
};
