#include "tools/texture_utils.h"

#include <bgfx/bgfx.h>
#include <bimg/bimg.h>
#include <bimg/decode.h>
#include <bx/file.h>
#include "core/logger.h"

namespace TextureUtils {

bgfx::TextureHandle LoadTexture(const std::string& filepath, bool srgb) {
  bx::DefaultAllocator allocator;
  bx::FileReader reader;

  // Try to open the texture file using BX's file reader
  if (!bx::open(&reader, filepath.c_str())) {
    Logger::getInstance().Log(LogLevel::Error,
                              "Failed to open texture: " + filepath);
    return BGFX_INVALID_HANDLE;
  }

  // Determine the file size and allocate memory to hold the entire file
  uint32_t size = (uint32_t)bx::getSize(&reader);
  const bgfx::Memory* mem = bgfx::alloc(size);

  // Read file contents into memory
  bx::read(&reader, mem->data, size, nullptr);
  bx::close(&reader);

  // Use bimg to parse the image from memory
  bimg::ImageContainer* image =
      bimg::imageParse(&allocator, mem->data, mem->size);
  if (!image) {
    Logger::getInstance().Log(LogLevel::Error,
                              "bimg failed to parse texture: " + filepath);
    return BGFX_INVALID_HANDLE;
  }

  // Configure texture flags
  uint64_t flags = BGFX_TEXTURE_NONE;
  if (srgb) {
    flags |= BGFX_TEXTURE_SRGB;  // Convert to sRGB if requested
  }

  // Create the actual texture using bgfx
  bgfx::TextureHandle handle = bgfx::createTexture2D(
      (uint16_t)image->m_width,   // Width
      (uint16_t)image->m_height,  // Height
      image->m_numMips > 1,       // Use mipmaps if available
      image->m_numLayers,         // Number of layers (usually 1)
      (bgfx::TextureFormat::Enum)image->m_format,  // Format decoded by bimg
      flags,                                       // Texture flags
      bgfx::copy(image->m_data, image->m_size)     // Copy the image data
  );

  // Check for failure in texture creation
  if (!bgfx::isValid(handle)) {
    Logger::getInstance().Log(LogLevel::Error,
                              "Failed to create texture: " + filepath);
  }

  // Clean up the parsed image structure
  bimg::imageFree(image);

  return handle;
}

}  // namespace TextureUtils
