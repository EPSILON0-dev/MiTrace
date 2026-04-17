#pragma once

#include <stb_image_write.h>

#include <glm/glm.hpp>
#include <memory>

// Note: We can more or less assume that no 2 threads will be writing to the same pixel at the same
// time, since each block is only rendered by one thread, so we don't need to worry about
// synchronization here
class RenderBuffer
{
   private:
    unsigned int width_ = 0;
    unsigned int height_ = 0;
    std::unique_ptr<glm::vec3[]> pixels_;  // RGB format

   public:
    RenderBuffer(unsigned width, unsigned height)
        : width_(width), height_(height), pixels_(new glm::vec3[width * height]())
    {
    }

    unsigned int GetWidth() const { return width_; }
    unsigned int GetHeight() const { return height_; }

    void GetPixel(unsigned x, unsigned y, glm::vec3& color) const;
    void SetPixel(unsigned x, unsigned y, const glm::vec3& color);
    void AddColorAt(unsigned x, unsigned y, const glm::vec3& color);
    void DrawSubBuffer(RenderBuffer& subBuffer, unsigned offsetX, unsigned offsetY);
    std::shared_ptr<uint8_t[]> GetPixelsRGB8() const;
    void SaveToFilePNG(const std::string& filename) const;
    void SaveToFileJPG(const std::string& filename) const;
    void SaveToFileHDR(const std::string& filename) const;
};
