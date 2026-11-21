/**
 * @file
 * @ingroup assets
 * @brief Persists per-texture color-space choices.
 *
 * Texture settings distinguish color data from linear data. Reimport must refresh dependent material bindings so existing scene materials use the rebuilt texture.
 *
 * @par Usage
 * Use sRGB for authored color textures and linear treatment for data textures. The intended interpretation belongs to the asset import settings.
 */
#pragma once
#include <Core/Core.h>
#include <filesystem>
#include <string>

namespace FINALITY
{
    /** @todo TODO: Document TextureColorSpace enum semantics, ownership and usage. */
    enum class TextureColorSpace { Automatic, SRGB, Linear };
    FAPI TextureColorSpace ReadTextureColorSpace(const std::filesystem::path& texture);
    FAPI bool SaveTextureColorSpace(const std::filesystem::path& texture, TextureColorSpace colorSpace, std::string& error);
}
