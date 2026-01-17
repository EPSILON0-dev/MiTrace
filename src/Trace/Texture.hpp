#pragma once

#include <memory>

#include "TextureImage.hpp"

class Texture
{
   private:
   TextureImage::FilterMode filterMode_;
   std::shared_ptr<TextureImage> image_;
   
   public:
    Texture(std::shared_ptr<TextureImage> image, TextureImage::FilterMode filter = TextureImage::FilterMode::Nearest)
        : filterMode_(filter), image_(image) {};

   public:
    glm::u8vec4 Sample(const glm::vec2& uv) const noexcept
    {
        return image_->Sample(uv, filterMode_);
    };

    glm::uvec4 SampleEquirectangular(const glm::vec3& direction) const noexcept
    {
        return image_->SampleEquirectangular(direction, filterMode_);
    };

    bool IsValid() const noexcept { return image_ != nullptr; }
};
