#include "Texture.hpp"

#include "CLI/Config.hpp"
#include "Common/ParallelForEach.hpp"

using namespace Scene;

std::map<std::string, std::shared_ptr<Image>> Texture::imageCache;

Texture::Texture(const Loader::Texture& texture)
{
    filterMode_ = texture.filter == Loader::SamplerFilter::Linear ? Image::FilterMode::Linear
                                                                  : Image::FilterMode::Nearest;
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

void Texture::LoadCachedImages()
{
    const auto nThread = Config::GetConfig().rendering.numThreads;
    Common::ParallelForEach(imageCache.begin(), imageCache.end(), nThread,
        [](const auto& cacheEntry) { cacheEntry.second->Load(); });
}
