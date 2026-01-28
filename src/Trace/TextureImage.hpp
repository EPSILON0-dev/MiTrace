/**
 * @file TextureImage.hpp
 * 
 * Texture image handling for loading and sampling texture data. It is 
 * separated from texture definitions as there can be multiple textures 
 * referencing the same image with different samplers.
 */
#pragma once

#include <filesystem>
#include <glm/glm.hpp>
#include <memory>
#include <string>

// For simplicity, we assume RGBA@8bpp with repeat wrap mode, no mipmaps and nearest/linear
// filtering options.
class TextureImage
{
   public:
    enum class FilterMode : uint8_t
    {
        Nearest,
        Linear
    };

   private:
    int width_, height_, channels_;
    std::string name_;
    std::filesystem::path filePath_;
    std::unique_ptr<glm::u8vec4[]> data_;

   public:
    TextureImage(const std::filesystem::path& filePath);

    auto GetWidth() const noexcept { return width_; }
    auto GetHeight() const noexcept { return height_; }
    auto GetChannels() const noexcept { return channels_; }
    const auto& GetName() const { return name_; }
    const auto& GetFilePath() const { return filePath_; }

    void SetName(const std::string& name) { name_ = name; }

   private:
    glm::u8vec4 SampleNearest(float& x, float& y) const noexcept;
    glm::u8vec4 SampleLinear(float& x, float& y) const noexcept;

   public:
    glm::u8vec4 Sample(const glm::vec2& uv, FilterMode filter) const noexcept;
    glm::u8vec4 SampleEquirectangular(const glm::vec3& direction, FilterMode filter) const noexcept;
};
