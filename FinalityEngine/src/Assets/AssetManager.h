#pragma once
#include <Assets/Asset.h>
#include <unordered_map>
#include <memory>
#include <string>

#include <Core/Core.h>

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
            auto it = s_PathToHandleRegistry.find(filePath);
            if (it != s_PathToHandleRegistry.end())
            {
                return std::static_pointer_cast<T>(s_LoadedAssets[it->second]);
            }

            std::shared_ptr<T> asset = T::Create(filePath, std::forward<Args>(args)...);
            if (!asset) return nullptr;

            UUID handle;
            asset->Handle = handle;

            s_LoadedAssets[handle] = asset;
            s_PathToHandleRegistry[filePath] = handle;

            return asset;
        }

        static std::shared_ptr<Asset> GetAsset(UUID handle);
        static bool HasAsset(UUID handle);

    private:
        static std::unordered_map<UUID, std::shared_ptr<Asset>> s_LoadedAssets;
        static std::unordered_map<std::string, UUID> s_PathToHandleRegistry;
    };
}
