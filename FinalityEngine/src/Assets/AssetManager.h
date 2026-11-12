#pragma once
#include <Assets/Asset.h>
#include <unordered_map>
#include <memory>
#include <string>

#include <Core/Core.h>
#include "AssetRegistry.h"

namespace FINALITY
{
    class FAPI AssetManager
    {
    public:
        static void Initialize();
        static void Shutdown();

        template<typename T, typename... Args>
        static std::shared_ptr<T> CreateAsset(const std::string& filePath, Args&&... args)
        {
            const std::string resolved = AssetRegistry::Resolve(filePath);
            if (resolved.empty()) return nullptr;
            auto it = s_PathToHandleRegistry.find(resolved);
            if (it != s_PathToHandleRegistry.end())
            {
                return std::dynamic_pointer_cast<T>(s_LoadedAssets[it->second]);
            }

            std::shared_ptr<T> asset;
            try { asset = T::Create(resolved, std::forward<Args>(args)...); }
            catch (const std::exception&) { return nullptr; }
            if (!asset) return nullptr;

            const auto* record = AssetRegistry::FindPath(resolved);
            UUID handle = record ? UUID(record->ID) : UUID();
            asset->Handle = handle;

            s_LoadedAssets[handle] = asset;
            s_PathToHandleRegistry[resolved] = handle;

            return asset;
        }

        static std::shared_ptr<Asset> GetAsset(UUID handle);
        static bool HasAsset(UUID handle);
        static size_t ReloadMaterialShaders(bool force = false);
        static void InvalidateAssetPath(const std::string& filePath);

    private:
        static std::unordered_map<UUID, std::shared_ptr<Asset>> s_LoadedAssets;
        static std::unordered_map<std::string, UUID> s_PathToHandleRegistry;
    };
}
