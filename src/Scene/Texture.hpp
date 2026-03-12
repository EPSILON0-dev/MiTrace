#pragma once

#include <memory>
#include <map>

#include "Image.hpp"
#include "Loader/Types.hpp"

namespace Scene
{

class Texture
{
   private:
    static std::map<std::string, std::shared_ptr<Image>> imageCache;
    std::shared_ptr<Image> image_;
    Image::FilterMode filterMode_;

   public:
    Texture() : image_(nullptr), filterMode_(Image::FilterMode::Nearest) {}
    Texture(const Loader::Texture& texture);

   public:
    glm::vec4 Sample(const glm::vec2& uv) const noexcept
    {
        return image_->Sample(uv, filterMode_);
    };

    glm::vec4 SampleEquirectangular(const glm::vec3& direction) const noexcept
    {
        return image_->SampleEquirectangular(direction, filterMode_);
    };

    bool IsValid() const noexcept { return image_->IsValid(); }

    static void LoadCachedImages();
    static void ClearCache() noexcept { imageCache.clear(); }
};

}  // namespace Scene
