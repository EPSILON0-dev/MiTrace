// TODO Come up with an interface for the lights, for now just a placeholder for the GLTF loader
#pragma once

#include <glm/glm.hpp>

class Light
{
   public:
    enum class Type
    {
        POINT,
        DIRECTIONAL,
        SPOT,
        AREA
    };

    struct PointLight
    {
        glm::vec3 Color;
        float Intensity;
        float Range;
        float Size;
    };

    struct DirectionalLight
    {
        glm::vec3 Color;
        float Intensity;
        float Angle;
    };

    struct SpotLight
    {
        glm::vec3 Color;
        float Intensity;
        float Range;
        float Size;
        float InnerConeAngle;
        float OuterConeAngle;
    };

    struct AreaLight
    {
        glm::vec3 Color;
        float Intensity;
        float Range;
        glm::vec2 Size;
    };

   private:
    glm::mat4 transform_;
    union
    {
        PointLight pointLight_;
        DirectionalLight directionalLight_;
        SpotLight spotLight_;
        AreaLight areaLight_;
    };
    Type type_;

   public:
    Light(const glm::mat4& transform, const PointLight& pointLight)
        : transform_(transform), pointLight_(pointLight), type_(Type::POINT) {};
    Light(const glm::mat4& transform, const DirectionalLight& directionalLight)
        : transform_(transform), directionalLight_(directionalLight), type_(Type::DIRECTIONAL) {};
    Light(const glm::mat4& transform, const SpotLight& spotLight)
        : transform_(transform), spotLight_(spotLight), type_(Type::SPOT) {};
    Light(const glm::mat4& transform, const AreaLight& areaLight)
        : transform_(transform), areaLight_(areaLight), type_(Type::AREA) {};
    ~Light() = default;

   public:
    Type GetType() const { return type_; }
    const PointLight& GetPointLight() const { return pointLight_; }
    const DirectionalLight& GetDirectionalLight() const { return directionalLight_; }
    const SpotLight& GetSpotLight() const { return spotLight_; }
    const AreaLight& GetAreaLight() const { return areaLight_; }
    const glm::mat4& GetTransform() const { return transform_; }
    const glm::vec3 GetPosition() const { return glm::vec3(transform_[3]); }
};
