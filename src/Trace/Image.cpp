#include "Image.hpp"

#include <stb/stb_image.h>
#include <glm/gtc/constants.hpp>

#include <memory>

// TODO move to templates

Image::Image(const std::filesystem::path& filePath)
    : width_(0), height_(0), channels_(0), filePath_(filePath), data_(nullptr)
{
    // Load the image using stb_image
    int n;
    unsigned char* imgData =
        stbi_load(filePath_.string().c_str(), &width_, &height_, &n, STBI_rgb_alpha);
    if (!imgData) throw std::runtime_error("Failed to load texture: " + filePath_.string());

    // Copy the image data into our own buffer
    channels_ = 4;  // We forced STBI_rgb_alpha
    size_t totalPixels = static_cast<size_t>(width_) * static_cast<size_t>(height_);
    data_ = std::make_unique<glm::u8vec4[]>(totalPixels);
    std::memcpy(data_.get(), imgData, totalPixels * sizeof(glm::u8vec4));
    stbi_image_free(imgData);
}

glm::u8vec4 Image::SampleNearest(float& x, float& y) const noexcept
{
    int ix = static_cast<int>(std::round(x * width_)) % width_;
    int iy = static_cast<int>(std::round(y * height_)) % height_;
    return data_[iy * width_ + ix];
}

glm::u8vec4 Image::SampleLinear(float& x, float& y) const noexcept
{
    int x0 = static_cast<int>(std::floor(x * width_));
    int x1 = std::min(x0 + 1, width_ - 1);
    int y0 = static_cast<int>(std::floor(y * height_));
    int y1 = std::min(y0 + 1, height_ - 1);

    float sx = x - static_cast<float>(x0) / width_;
    float sy = y - static_cast<float>(y0) / height_;

    glm::u8vec4 c00 = data_[y0 * width_ + x0];
    glm::u8vec4 c10 = data_[y0 * width_ + x1];
    glm::u8vec4 c01 = data_[y1 * width_ + x0];
    glm::u8vec4 c11 = data_[y1 * width_ + x1];

    glm::u8vec4 c0 = glm::mix(c00, c10, sx);
    glm::u8vec4 c1 = glm::mix(c01, c11, sx);
    return glm::mix(c0, c1, sy);
}

glm::u8vec4 Image::Sample(const glm::vec2& uv, const FilterMode filter) const noexcept
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
            return glm::u8vec4(0, 0, 0, 255);
    }
}

glm::u8vec4 Image::SampleEquirectangular(
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
