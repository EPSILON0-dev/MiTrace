#include "Texture.hpp"

#include <spdlog/spdlog.h>

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

Texture Texture::LoadEnvTexture(const std::filesystem::path& path)
{
    Loader::Image loaderImage = {.name = "env_texture", .path = path};
    Scene::Image hdriImage(loaderImage);
    hdriImage.Load();
    if (!hdriImage.IsValid())
    {
        spdlog::critical("Failed to load environment texture");
        std::exit(EXIT_FAILURE);
    }
    Texture tex;
    tex.image_ = std::make_shared<Image>(hdriImage);
    tex.filterMode_ = Image::FilterMode::Linear;
    tex.offset_ = glm::vec2{Config::GetConfig().hdriRotation / 360.0f, 0.0f};
    return tex;
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
    const auto nThread = Config::GetConfig().numThreads;
    Common::ParallelForEach(imageCache.begin(), imageCache.end(), nThread,
        [](const auto& cacheEntry) { cacheEntry.second->Load(); });
}
