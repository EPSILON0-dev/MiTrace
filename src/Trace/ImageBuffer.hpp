#pragma once

#include <memory>
#include <glm/glm.hpp>

namespace Trace { class ImageBuffer; }

class Trace::ImageBuffer
{
   private:
    unsigned int width_ = 0;
    unsigned int height_ = 0;
    std::unique_ptr<unsigned char[]> pixels_;  // RGB format

   public:
    ImageBuffer(unsigned width, unsigned height)
        : width_(width), height_(height), pixels_(new unsigned char[width * height * 3]())
    {
    }

    glm::u8vec4 GetPixel(unsigned x, unsigned y) const
    {
        if (x >= width_ || y >= height_)
            return glm::u8vec4(0, 0, 0, 255);
        unsigned int index = (y * width_ + x) * 3;
        return glm::u8vec4(pixels_[index + 0], pixels_[index + 1], pixels_[index + 2], 255);
    }

    void SetPixel(unsigned x, unsigned y, unsigned char r, unsigned char g, unsigned char b)
    {
        if (x >= width_ || y >= height_)
            return;
        unsigned int index = (y * width_ + x) * 3;
        pixels_[index + 0] = r;
        pixels_[index + 1] = g;
        pixels_[index + 2] = b;
    }

    unsigned int GetWidth() const { return width_; }
    unsigned int GetHeight() const { return height_; }
    const unsigned char* GetPixels() const { return pixels_.get(); }
    unsigned char* GetPixels() { return pixels_.get(); }
};
