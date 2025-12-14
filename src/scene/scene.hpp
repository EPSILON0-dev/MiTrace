#pragma once

#include <glm/glm.hpp>
#include <vector>

class Scene
{
   private:
    std::vector<glm::vec4> spheres;

   public:
    Scene() noexcept {}

    void AddSphere(const glm::vec3& position, float size) noexcept
    {
        spheres.push_back(glm::vec4(position.x, position.y, position.z, size));
    }

    const std::vector<glm::vec4>& GetSpheres() const noexcept
    {
        return spheres;
    }
};
