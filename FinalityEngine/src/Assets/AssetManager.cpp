#include "AssetManager.h"

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
