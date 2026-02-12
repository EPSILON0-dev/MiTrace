#pragma once

#include <spdlog/spdlog.h>

#include <glm/glm.hpp>

#include "Loader/Types.hpp"

namespace Scene
{

class Light
{
   private:
    glm::vec3 position;
    glm::vec3 color;

   public:
    Light(const Loader::Light& light)
        : position(light.transform[3]), color(light.color * light.intensity)
    {
        if (light.type != Loader::LightType::Point)
            spdlog::warn("Only point lights are supported, but got a different type");
    }

    glm::vec3 GetPosition() const noexcept { return position; }
    glm::vec3 GetColor() const noexcept { return color; }
};

}  // namespace Scene
