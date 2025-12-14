#pragma once

namespace GUI
{
class GLTexture;
}

class GUI::GLTexture
{
   private:
    unsigned int textureID_ = 0;
    int width_ = 0;
    int height_ = 0;

   public:
    GLTexture(unsigned width, unsigned height, const unsigned char* rgbPixelsU8);
    ~GLTexture();

    // Disable copy
    GLTexture(const GLTexture&) = delete;
    GLTexture& operator=(const GLTexture&) = delete;
    // Allow move
    GLTexture(GLTexture&& other) noexcept = default;
    GLTexture& operator=(GLTexture&& other) noexcept = default;

    unsigned int GetID() const { return textureID_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
};
