#pragma once

#include <memory>

#include "Image.hpp"

class Texture
{
   private:
   Image::FilterMode filterMode_;
   std::shared_ptr<Image> image_;
   
   public:
    Texture(std::shared_ptr<Image> image, Image::FilterMode filter = Image::FilterMode::Nearest)
        : filterMode_(filter), image_(image) {};

   public:
    glm::u8vec4 Sample(const glm::vec2& uv) const noexcept
    {
        return image_->Sample(uv, filterMode_);
    };
};
