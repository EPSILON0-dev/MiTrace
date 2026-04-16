#include "Texture.hpp"

#include "CLI/Config.hpp"
#include "Common/ParallelForEach.hpp"

using namespace Scene;

std::map<std::string, std::shared_ptr<Image>> Texture::imageCache;

glm::vec2 Texture::TransformUv(const glm::vec2& uv) const noexcept
{
    return uv * scale_ + offset_;
}

Texture::Texture(const Loader::Texture& texture)
{
    filterMode_ = texture.filter == Loader::SamplerFilter::Linear ? Image::FilterMode::Linear
                                                                  : Image::FilterMode::Nearest;
    offset_ = texture.offset;
    scale_ = texture.scale;
    auto it = imageCache.find(texture.image.path);
    if (it != imageCache.end())
    {
        image_ = it->second;
    }
    else
    {
        image_ = std::make_shared<Image>(texture.image);
        imageCache[texture.image.path] = image_;
    }
}

glm::vec4 Texture::Sample(const glm::vec2& uv) const noexcept
{
    if (!image_ || !image_->IsValid()) return glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};
    return image_->Sample(TransformUv(uv), filterMode_);
}

glm::vec4 Texture::SampleEquirectangular(const glm::vec3& direction) const noexcept
{
    return image_->SampleEquirectangular(direction, filterMode_);
}

void Texture::LoadCachedImages()
{
    const auto nThread = Config::GetConfig().rendering.numThreads;
    Common::ParallelForEach(imageCache.begin(), imageCache.end(), nThread,
        [](const auto& cacheEntry) { cacheEntry.second->Load(); });
}
