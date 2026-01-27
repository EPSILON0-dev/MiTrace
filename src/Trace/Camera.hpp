/**
 * @file Camera.hpp
 * 
 * Camera representation for generating rays based on camera parameters.
 */
#pragma once

#include <glm/fwd.hpp>

#include "Ray.hpp"

class Camera
{
   private:
    glm::mat4 cameraToWorld_;
    float yfovRadians_;

   public:
    Camera() noexcept : yfovRadians_(60.0f) {};

    Camera(float yfov, const glm::mat4& cameraToWorld = glm::mat4(1.0f)) noexcept
        : cameraToWorld_(cameraToWorld), yfovRadians_(yfov) {};

   public:
    glm::mat4 GetCameraToWorld() noexcept { return cameraToWorld_; }
    float GetYFovRadians() noexcept { return yfovRadians_; }

   public:
    void SetCameraToWorld(const glm::mat4& cameraToWorld) noexcept
    {
        cameraToWorld_ = cameraToWorld;
    }
    void SetYFovRadians(float yfov) noexcept { yfovRadians_ = yfov; }

   public:
    // UV coordinates are in range [0, 1]
    Ray GenerateRay(float u, float v, float aspectRatio) const noexcept;
};
