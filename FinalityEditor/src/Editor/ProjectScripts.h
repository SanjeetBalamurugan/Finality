#pragma once

#include <Project/Project.h>
#include <Scene/ScriptModule.h>
#include <Core/Process.h>
#include <future>
#include <chrono>

namespace FINALITY
{
    class ProjectScripts
    {
    public:
        void Open(const Project& project, const std::filesystem::path& engineAssets);
        void Close();
        void Update(const Project& project, Scene* scene, bool canReload);
        void Build() { m_RequestBuild = true; }
        void Reload() { m_PendingReload = true; }
        bool IsBuilding() const { return m_Build.valid(); }
        const std::string& GetStatus() const { return m_Status; }
        const std::string& GetDiagnostics() const { return m_Diagnostics; }
        uint64_t GetGeneration() const { return m_Module.GetGeneration(); }

    private:
        uint64_t Fingerprint() const;
        ScriptModule m_Module;
        std::filesystem::path m_Root, m_EngineRoot, m_EngineBin, m_Source, m_Library;
        std::future<ProcessResult> m_Build;
        uint64_t m_Observed = 0, m_Built = 0, m_Building = 0;
        bool m_RequestBuild = false, m_PendingReload = false;
        std::chrono::steady_clock::time_point m_Changed{}, m_NextScan{};
        std::string m_Status = "No project scripts loaded", m_Diagnostics;
    };
}
