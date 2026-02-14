#pragma once

#include <spdlog/spdlog.h>

#include <glm/glm.hpp>

#include "Loader/Types.hpp"
#include "glm/fwd.hpp"

namespace Scene
{

class Light
{
   private:
    glm::vec3 position_;
    glm::vec3 color_;
    float pointSize_;

   public:
    Light(const Loader::Light& light)
        : position_(light.transform[3]),
          color_(light.color * light.intensity),
          pointSize_(light.pointSize)
    {
        if (light.type != Loader::LightType::Point)
            spdlog::warn("Only point lights are supported, but got a different type");
    }

    const glm::vec3& GetPosition() const noexcept { return position_; }
    float GetPointSize() const noexcept { return pointSize_; }
    const glm::vec3& GetColor() const noexcept { return color_; }
};

}  // namespace Scene
