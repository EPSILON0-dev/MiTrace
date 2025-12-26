#pragma once

#include <filesystem>
#include <glm/glm.hpp>
#include <memory>
#include <string>

// For simplicity, we assume RGBA@8bpp with repeat wrap mode, no mipmaps and nearest/linear
// filtering options.
class Image
{
    friend class GLTF_Loader;

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
    Image(const std::filesystem::path& filePath);

    int GetWidth() const noexcept { return width_; }
    int GetHeight() const noexcept { return height_; }
    int GetChannels() const noexcept { return channels_; }
    const std::string& GetName() const { return name_; }
    const std::filesystem::path& GetFilePath() const { return filePath_; }

   private:
    glm::u8vec4 SampleNearest(float& x, float& y) const noexcept;
    glm::u8vec4 SampleLinear(float& x, float& y) const noexcept;

   public:
    glm::u8vec4 Sample(const glm::vec2& uv, FilterMode filter) const noexcept;
    glm::u8vec4 SampleEquirectangular(const glm::vec3& direction, FilterMode filter) const noexcept;
};
