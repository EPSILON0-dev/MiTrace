#pragma once

#include <spdlog/spdlog.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Loader/Types.hpp"
#include "glm/fwd.hpp"

namespace Scene
{

class Light
{
   public:
    enum class LightType : uint8_t
    {
        Point,
        Directional,
        Spot,
        Area
    };

   private:
    LightType type_;
    glm::vec3 position_;
    glm::quat quatDirection_;
    glm::vec3 vecDirection_;
    glm::vec3 color_;
    float pointSize_;
    float spotInnerConeDot_;
    float spotOuterConeDot_;

   public:
    Light(const Loader::Light& light)
        : type_(static_cast<LightType>(light.type)),
          position_(light.transform[3]),
          color_(light.color * light.intensity),
          pointSize_(light.pointSize),
          spotInnerConeDot_(glm::cos(light.spotInnerConeAngle)),
          spotOuterConeDot_(glm::cos(light.spotOuterConeAngle))
    {
        if (light.type != Loader::LightType::Point && light.type != Loader::LightType::Spot)
            spdlog::warn("Only point and spot lights are supported, but got a different type");

        if (light.type == Loader::LightType::Directional || light.type == Loader::LightType::Spot)
        {
            const auto rotationMatrix = glm::mat3(light.transform);
            quatDirection_ = glm::quat_cast(rotationMatrix);
            vecDirection_ = quatDirection_ * glm::vec3(0.0f, 0.0f, -1.0f);
        }
    }

    auto GetType() const noexcept { return type_; }
    const glm::vec3& GetPosition() const noexcept { return position_; }
    // const glm::quat& GetDirection() const noexcept { return quatDirection_; }
    float GetSpotIntensity(const glm::vec3& dir) const noexcept
    {
        float dot = glm::dot(-dir, vecDirection_);
        float intensity = (dot - spotOuterConeDot_) / (spotInnerConeDot_ - spotOuterConeDot_);
        return glm::clamp(intensity, 0.0f, 1.0f);
    }
    float GetPointSize() const noexcept { return pointSize_; }
    const glm::vec3& GetColor() const noexcept { return color_; }
};

}  // namespace Scene
