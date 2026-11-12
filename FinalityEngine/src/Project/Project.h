#pragma once

#include <Core/Core.h>
#include <filesystem>
#include <string>
#include "ProjectSettings.h"

namespace FINALITY
{
    class FAPI Project
    {
    public:
        bool Create(const std::filesystem::path& location, const std::string& name,
            const std::filesystem::path& templateAssets = {}, bool starterTemplate = true);
        bool Open(const std::filesystem::path& path);
        bool Save() const;
        bool SaveSettings() const;
        bool LoadSettings();
        void SetName(const std::string& name) { if (!name.empty()) m_Name = name; }
        bool IsOpen() const { return !m_Path.empty(); }
        const std::filesystem::path& GetPath() const { return m_Path; }
        std::filesystem::path GetDirectory() const { return m_Path.parent_path(); }
        std::filesystem::path GetAssetsDirectory() const { return GetDirectory() / "Assets"; }
        const std::string& GetName() const { return m_Name; }
        std::string StartupScene = "Assets/Scenes/Main.fscene";
        ProjectSettings Settings;

    private:
        std::filesystem::path m_Path;
        std::string m_Name;
    };
}
