#include "Image.hpp"

#include <spdlog/spdlog.h>
#include <stb/stb_image.h>

#include <glm/gtc/constants.hpp>
#include <stdexcept>

using namespace Scene;

glm::vec4 Image::SamplePixel(int x, int y) const noexcept
{
    auto idx = (y * width_ + x) * 3;
    return glm::vec4(data_[idx], data_[idx + 1], data_[idx + 2], 255) / 255.0f;
}

glm::vec4 Image::SampleNearest(float& x, float& y) const noexcept
{
    int ix = static_cast<int>(x * static_cast<float>(width_)) % width_;
    int iy = static_cast<int>(y * static_cast<float>(height_)) % height_;
    if (ix < 0) ix += width_;
    if (iy < 0) iy += height_;
    return SamplePixel(ix, iy);
}

glm::vec4 Image::SampleLinear(float& x, float& y) const noexcept
{
    int x0 = static_cast<int>(std::floor(x * static_cast<float>(width_)));
    int x1 = std::min(x0 + 1, width_ - 1);
    int y0 = static_cast<int>(std::floor(y * static_cast<float>(height_)));
    int y1 = std::min(y0 + 1, height_ - 1);

    float sx = x - static_cast<float>(x0) / static_cast<float>(width_);
    float sy = y - static_cast<float>(y0) / static_cast<float>(height_);

    auto c00 = SamplePixel(x0, y0);
    auto c10 = SamplePixel(x1, y0);
    auto c01 = SamplePixel(x0, y1);
    auto c11 = SamplePixel(x1, y1);

    auto c0 = glm::mix(c00, c10, sx);
    auto c1 = glm::mix(c01, c11, sx);
    return glm::mix(c0, c1, sy);
}

glm::vec4 Image::Sample(const glm::vec2& uv, const FilterMode filter) const noexcept
{
    float u = uv.x - std::floor(uv.x);
    float v = uv.y - std::floor(uv.y);

    // Unfortunately, since this is a very hot path, we cannot throw an exception here.
    switch (filter)
    {
        case FilterMode::Nearest:
            return SampleNearest(u, v);
        case FilterMode::Linear:
            return SampleLinear(u, v);
        default:
            return {0.0f, 0.0f, 0.0f, 1.0f};
    }
}

glm::vec4 Image::SampleEquirectangular(
    const glm::vec3& direction, const FilterMode filter) const noexcept
{
    // Convert direction to spherical coordinates
    float theta = std::atan2(direction.z, direction.x);           // Longitude
    float phi = std::acos(direction.y / glm::length(direction));  // Latitude

    // Map spherical coordinates to [0, 1] range
    float u = (theta + glm::pi<float>()) / (2.0f * glm::pi<float>());
    float v = phi / glm::pi<float>();

    return Sample(glm::vec2(u, v), filter);
}

void Image::Load()
{
    if (path_ == "") return;

    const auto isFloatImage = path_.ends_with(".hdr") || path_.ends_with(".exr");
    const auto basename = path_.substr(path_.rfind('/') + 1);

    uint8_t* pixels = nullptr;
    if (isFloatImage)
    {
        spdlog::error("Cannot load float image {}", basename);
        throw std::runtime_error("Cannot load float image");
    }
    else
    {
        FILE* f = fopen(path_.c_str(), "rb");
        pixels = stbi_load_from_file(f, &width_, &height_, &channels_, 3);
        fclose(f);
    }

    // Move the loaded image data into the Image object
    if (pixels == nullptr)
        throw std::runtime_error(std::format("Failed to load image: {}", basename));
    data_ = std::shared_ptr<uint8_t[]>(pixels, stbi_image_free);

    spdlog::debug(
        "Loaded new image \"{}\" ({}x{}, {} channels)", basename, width_, height_, channels_);
}
