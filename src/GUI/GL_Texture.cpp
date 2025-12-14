#include "GL/glew.h"
#include "GL_Texture.hpp"

GUI::GLTexture::GLTexture(unsigned width, unsigned height, const unsigned char* rgbPixelsU8)
    : width_(width), height_(height)
{
    glGenTextures(1, &textureID_);
    glBindTexture(GL_TEXTURE_2D, textureID_);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width_, height_, 0, GL_RGB, GL_UNSIGNED_BYTE, rgbPixelsU8);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}

GUI::GLTexture::~GLTexture()
{
    glDeleteTextures(1, &textureID_);
}
