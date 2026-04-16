#pragma once

#include <glm/glm.hpp>
#include <map>
#include <memory>

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
    glm::vec2 offset_;
    glm::vec2 scale_;

    glm::vec2 TransformUv(const glm::vec2& uv) const noexcept;

   public:
    Texture()
        : image_(nullptr), filterMode_(Image::FilterMode::Nearest), offset_(0.0f), scale_(1.0f)
    {
    }
    Texture(const Loader::Texture& texture);

   public:
    glm::vec4 Sample(const glm::vec2& uv) const noexcept;
    glm::vec4 SampleEquirectangular(const glm::vec3& direction) const noexcept;
    bool IsValid() const noexcept { return image_ && image_->IsValid(); }

    static void LoadCachedImages();
    static void ClearCache() noexcept { imageCache.clear(); }
};

}  // namespace Scene
