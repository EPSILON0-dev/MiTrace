#pragma once

#include <memory>

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
