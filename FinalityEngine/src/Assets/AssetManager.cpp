#include "AssetManager.h"
#include "MaterialAsset.h"
#include <vector>

namespace FINALITY
{
    std::unordered_map<UUID, std::shared_ptr<Asset>> AssetManager::s_LoadedAssets;
    std::unordered_map<std::string, UUID> AssetManager::s_PathToHandleRegistry;

    void AssetManager::Initialize()
    {
        s_LoadedAssets.clear();
        s_PathToHandleRegistry.clear();
    }

    void AssetManager::Shutdown()
    {
        s_LoadedAssets.clear();
        s_PathToHandleRegistry.clear();
    }

    std::shared_ptr<Asset> AssetManager::GetAsset(UUID handle)
    {
        auto it = s_LoadedAssets.find(handle);
        if (it != s_LoadedAssets.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool AssetManager::HasAsset(UUID handle)
    {
        return s_LoadedAssets.find(handle) != s_LoadedAssets.end();
    }
}

size_t FINALITY::AssetManager::ReloadMaterialShaders(bool force)
{
    size_t rebuilt = 0;
    for (auto& [handle, asset] : s_LoadedAssets)
    {
        if (auto material = std::dynamic_pointer_cast<MaterialAsset>(asset))
            rebuilt += material->ReloadShadersIfChanged(force) ? 1 : 0;
    }
    return rebuilt;
}

void FINALITY::AssetManager::InvalidateAssetPath(const std::string& filePath)
{
    auto it = s_PathToHandleRegistry.find(filePath);
    if (it == s_PathToHandleRegistry.end()) return;
    s_LoadedAssets.erase(it->second);
    s_PathToHandleRegistry.erase(it);
}
