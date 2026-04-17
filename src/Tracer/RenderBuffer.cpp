#include "RenderBuffer.hpp"

void RenderBuffer::SetPixel(unsigned x, unsigned y, const glm::vec3& color)
{
    if (x >= width_ || y >= height_) return;
    unsigned int index = (y * width_ + x);
    pixels_[index] = color;
}

void RenderBuffer::GetPixel(unsigned x, unsigned y, glm::vec3& color) const
{
    if (x >= width_ || y >= height_) return;
    unsigned int index = (y * width_ + x);
    color = pixels_[index];
}

void RenderBuffer::AddColorAt(unsigned x, unsigned y, const glm::vec3& color)
{
    if (x >= width_ || y >= height_) return;
    unsigned int index = (y * width_ + x);
    pixels_[index] += color;
}

void RenderBuffer::DrawSubBuffer(RenderBuffer& subBuffer, unsigned offsetX, unsigned offsetY)
{
    for (unsigned int y = 0; y < subBuffer.GetHeight(); ++y)
    {
        for (unsigned int x = 0; x < subBuffer.GetWidth(); ++x)
        {
            glm::vec3 color;
            subBuffer.GetPixel(x, y, color);
            SetPixel(offsetX + x, offsetY + y, color);
        }
    }
}

static uint8_t FloatToU8(float v) noexcept
{
    return static_cast<uint8_t>(glm::clamp(v * 255.0f, 0.0f, 255.0f));
}

std::shared_ptr<uint8_t[]> RenderBuffer::GetPixelsRGB8() const
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

void RenderBuffer::SaveToFilePNG(const std::string& filename) const
{
    auto rgbPixels = GetPixelsRGB8();
    // 1 means success, there's no define for it
    if (stbi_write_png(filename.c_str(), static_cast<int>(width_), static_cast<int>(height_), 3,
            rgbPixels.get(), static_cast<int>(width_ * 3)) != 1)
        throw std::runtime_error("Failed to save image to file");
}

void RenderBuffer::SaveToFileJPG(const std::string& filename) const
{
    auto rgbPixels = GetPixelsRGB8();
    // 1 means success, there's no define for it
    if (stbi_write_jpg(filename.c_str(), static_cast<int>(width_), static_cast<int>(height_), 3,
            rgbPixels.get(), static_cast<int>(width_ * 3)) != 1)
        throw std::runtime_error("Failed to save image to file");
}

void RenderBuffer::SaveToFileHDR(const std::string& filename) const
{
    // 1 means success, there's no define for it
    if (stbi_write_hdr(filename.c_str(), static_cast<int>(width_), static_cast<int>(height_), 3,
            reinterpret_cast<float*>(pixels_.get())) != 1)
        throw std::runtime_error("Failed to save image to file");
}
