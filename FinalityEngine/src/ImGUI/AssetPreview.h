#pragma once

#include <Core/Core.h>
#include <string>
#include <Assets/AssetRegistry.h>

namespace FINALITY
{
    class FAPI AssetPreview
    {
    public:
        static void* TextureHandle(const std::string& reference);
        static void* RenderedHandle(const std::string& reference, ProjectAssetType type);
        static void Update();
        static void Configure(int resolution, int capacity);
        static void Clear();
        static void Flush();
    };
}
