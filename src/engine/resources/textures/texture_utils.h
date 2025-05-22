#ifndef TEXTURE_UTILS_H
#define TEXTURE_UTILS_H

#include <bgfx/bgfx.h>
#include <string>

namespace TextureUtils {
  bgfx::TextureHandle LoadTexture(const std::string& filepath, bool srgb = false);
}

#endif  // TEXTURE_UTILS_H
