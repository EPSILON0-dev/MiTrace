#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "Loader/Types.hpp"

namespace Scene
{

class Image
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
    std::shared_ptr<uint8_t[]> data_;

   public:
    Image(const Loader::Image& image)
    {
        width_ = image.width;
        height_ = image.height;
        channels_ = image.channels;
        data_ = image.pixels;
        name_ = image.name;

        if (channels_ != 3)
        {
            throw std::runtime_error("Only RGB images are supported");
        }
    }

    auto GetWidth() const noexcept { return width_; }
    auto GetHeight() const noexcept { return height_; }
    auto GetChannels() const noexcept { return channels_; }
    const uint8_t* GetPixels() const noexcept { return data_.get(); }
    const auto& GetName() const noexcept { return name_; }

    void SetName(const std::string& name) { name_ = name; }

   private:
    glm::vec4 SamplePixel(int x, int y) const noexcept;
    glm::vec4 SampleNearest(float& x, float& y) const noexcept;
    glm::vec4 SampleLinear(float& x, float& y) const noexcept;

   public:
    glm::vec4 Sample(const glm::vec2& uv, FilterMode filter) const noexcept;
    glm::vec4 SampleEquirectangular(const glm::vec3& direction, FilterMode filter) const noexcept;
};

}  // namespace Scene
