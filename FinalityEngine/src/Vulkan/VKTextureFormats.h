/**
 * @file
 * @ingroup vulkan
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for VKTextureFormats.h.
 */
#pragma once
#include "VKCore.h"
#include <gli/format.hpp>
#include <stdexcept>
#include <Assets/TextureImportSettings.h>

namespace FINALITY
{
    inline VkFormat TextureColorFormat(VkFormat format, TextureColorSpace colorSpace)
    {
        if (colorSpace == TextureColorSpace::Automatic) return format;
        const bool srgb = colorSpace == TextureColorSpace::SRGB;
        switch (format)
        {
        case VK_FORMAT_R8G8B8A8_UNORM: case VK_FORMAT_R8G8B8A8_SRGB: return srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
        case VK_FORMAT_B8G8R8A8_UNORM: case VK_FORMAT_B8G8R8A8_SRGB: return srgb ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM;
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK: case VK_FORMAT_BC1_RGB_SRGB_BLOCK: return srgb ? VK_FORMAT_BC1_RGB_SRGB_BLOCK : VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return srgb ? VK_FORMAT_BC1_RGBA_SRGB_BLOCK : VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case VK_FORMAT_BC2_UNORM_BLOCK: case VK_FORMAT_BC2_SRGB_BLOCK: return srgb ? VK_FORMAT_BC2_SRGB_BLOCK : VK_FORMAT_BC2_UNORM_BLOCK;
        case VK_FORMAT_BC3_UNORM_BLOCK: case VK_FORMAT_BC3_SRGB_BLOCK: return srgb ? VK_FORMAT_BC3_SRGB_BLOCK : VK_FORMAT_BC3_UNORM_BLOCK;
        case VK_FORMAT_BC7_UNORM_BLOCK: case VK_FORMAT_BC7_SRGB_BLOCK: return srgb ? VK_FORMAT_BC7_SRGB_BLOCK : VK_FORMAT_BC7_UNORM_BLOCK;
        default:
            if (srgb) throw std::runtime_error("This data texture format has no sRGB variant");
            return format;
        }
    }

    inline VkFormat TextureFormat(gli::format format)
    {
        switch (format)
        {
        case gli::FORMAT_RGBA8_UNORM_PACK8: return VK_FORMAT_R8G8B8A8_UNORM;
        case gli::FORMAT_RGBA8_SRGB_PACK8: return VK_FORMAT_R8G8B8A8_SRGB;
        case gli::FORMAT_BGRA8_UNORM_PACK8: return VK_FORMAT_B8G8R8A8_UNORM;
        case gli::FORMAT_BGRA8_SRGB_PACK8: return VK_FORMAT_B8G8R8A8_SRGB;
        case gli::FORMAT_RGB_DXT1_UNORM_BLOCK8: return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        case gli::FORMAT_RGB_DXT1_SRGB_BLOCK8: return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
        case gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case gli::FORMAT_RGBA_DXT1_SRGB_BLOCK8: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
        case gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16: return VK_FORMAT_BC2_UNORM_BLOCK;
        case gli::FORMAT_RGBA_DXT3_SRGB_BLOCK16: return VK_FORMAT_BC2_SRGB_BLOCK;
        case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16: return VK_FORMAT_BC3_UNORM_BLOCK;
        case gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16: return VK_FORMAT_BC3_SRGB_BLOCK;
        case gli::FORMAT_R_ATI1N_UNORM_BLOCK8: return VK_FORMAT_BC4_UNORM_BLOCK;
        case gli::FORMAT_R_ATI1N_SNORM_BLOCK8: return VK_FORMAT_BC4_SNORM_BLOCK;
        case gli::FORMAT_RG_ATI2N_UNORM_BLOCK16: return VK_FORMAT_BC5_UNORM_BLOCK;
        case gli::FORMAT_RG_ATI2N_SNORM_BLOCK16: return VK_FORMAT_BC5_SNORM_BLOCK;
        case gli::FORMAT_RGB_BP_UFLOAT_BLOCK16: return VK_FORMAT_BC6H_UFLOAT_BLOCK;
        case gli::FORMAT_RGB_BP_SFLOAT_BLOCK16: return VK_FORMAT_BC6H_SFLOAT_BLOCK;
        case gli::FORMAT_RGBA_BP_UNORM_BLOCK16: return VK_FORMAT_BC7_UNORM_BLOCK;
        case gli::FORMAT_RGBA_BP_SRGB_BLOCK16: return VK_FORMAT_BC7_SRGB_BLOCK;
        default: throw std::runtime_error("Unsupported texture format; use RGBA8 or BC1-BC7 DDS/KTX");
        }
    }
}
