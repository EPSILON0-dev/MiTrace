#pragma once

#include <stb/stb_image_write.h>
#include <glm/glm.hpp>
#include <memory>

class RenderBuffer
{
   private:
    unsigned int width_ = 0;
    unsigned int height_ = 0;
    std::unique_ptr<glm::vec3[]> pixels_;  // RGBA format

   public:
    RenderBuffer(unsigned width, unsigned height)
        : width_(width), height_(height), pixels_(new glm::vec3[width * height]())
    {
    }

    void SetPixel(unsigned x, unsigned y, const glm::vec3& color)
    {
        if (x >= width_ || y >= height_) return;
        unsigned int index = (y * width_ + x);
        pixels_[index] = color;
    }

    void GetPixel(unsigned x, unsigned y, glm::vec3& color) const
    {
        if (x >= width_ || y >= height_) return;
        unsigned int index = (y * width_ + x);
        color = pixels_[index];
    }

    void AddColorAt(unsigned x, unsigned y, const glm::vec3& color)
    {
        if (x >= width_ || y >= height_) return;
        unsigned int index = (y * width_ + x);
        pixels_[index] += color;
    }

    uint8_t FloatToU8(float v) const
    {
        return static_cast<uint8_t>(glm::clamp(v * 255.0f, 0.0f, 255.0f));
    }

    std::shared_ptr<uint8_t[]> GetPixelsRGB8() const
    {
        std::shared_ptr<uint8_t[]> rgbPixels(new uint8_t[width_ * height_ * 3]);

        for (unsigned int y = 0; y < height_; ++y)
        {
            for (unsigned int x = 0; x < width_; ++x)
            {
                unsigned int index = (y * width_ + x);
                rgbPixels[(index * 3) + 0] = FloatToU8(pixels_[index].r);
                rgbPixels[(index * 3) + 1] = FloatToU8(pixels_[index].g);
                rgbPixels[(index * 3) + 2] = FloatToU8(pixels_[index].b);
            }
        }

        return rgbPixels;
    }

    unsigned int GetWidth() const { return width_; }
    unsigned int GetHeight() const { return height_; }

    void SaveToFile(const std::string &filename) const
    {
        auto rgbPixels = GetPixelsRGB8();
        // 1 means success, there's no define for it
        if (stbi_write_png(filename.c_str(), width_, height_, 3, rgbPixels.get(), width_ * 3) != 1)
            throw std::runtime_error("Failed to save image to file");
    }
};
