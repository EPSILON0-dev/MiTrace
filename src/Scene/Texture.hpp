#pragma once

#include "Image.hpp"
#include "Loader/Types.hpp"

namespace Scene
{

class Texture
{
   private:
    Image::FilterMode filterMode_;
    std::optional<Image> image_;

   public:
    Texture() : filterMode_(Image::FilterMode::Nearest), image_(std::nullopt) {}
    Texture(const Loader::Texture& texture)
    {
        filterMode_ = texture.filter == Loader::SamplerFilter::Linear ? Image::FilterMode::Linear
                                                                      : Image::FilterMode::Nearest;
        image_ = std::nullopt;
        if (texture.image != nullptr) image_ = Image(*texture.image);
    }

   public:
    glm::vec4 Sample(const glm::vec2& uv) const noexcept
    {
        return image_->Sample(uv, filterMode_);
    };

    glm::vec4 SampleEquirectangular(const glm::vec3& direction) const noexcept
    {
        return image_->SampleEquirectangular(direction, filterMode_);
    };

    bool IsValid() const noexcept { return image_.has_value(); }
};

}  // namespace Scene
