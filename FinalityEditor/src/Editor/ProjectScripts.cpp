#include "ProjectScripts.h"
#include <Finality.h>
#include <fstream>

namespace
{
#ifdef NDEBUG
    constexpr const wchar_t* Configuration = L"Release";
#else
    constexpr const wchar_t* Configuration = L"Debug";
#endif
}

uint64_t FINALITY::ProjectScripts::Fingerprint() const
{
    uint64_t value = 1469598103934665603ull;
    std::error_code error;
    if (!std::filesystem::is_directory(m_Source,error)) return 0;
    for (std::filesystem::recursive_directory_iterator it(m_Source,std::filesystem::directory_options::skip_permission_denied,error),end; it != end && !error; it.increment(error))
    {
        if (!it->is_regular_file()) continue;
        const auto extension = it->path().extension();
        if (extension != ".cpp" && extension != ".h" && extension != ".hpp" && extension != ".inl") continue;
        const auto key = it->path().string() + std::to_string(it->last_write_time().time_since_epoch().count()) + std::to_string(it->file_size());
        for (unsigned char c : key) { value ^= c; value *= 1099511628211ull; }
    }
    return value;
}

void FINALITY::ProjectScripts::Open(const Project& project, const std::filesystem::path& engineAssets)
{
    Close();
    m_Root = project.GetDirectory(); m_Source = m_Root / project.Settings.ScriptDirectory;
    m_EngineBin = Process::ExecutablePath().parent_path();
    for (auto directory = engineAssets; !directory.empty(); directory = directory.parent_path())
    {
        if (std::filesystem::is_regular_file(directory / "Scripts/NativeModule.ps1")) { m_EngineRoot = directory; break; }
        if (directory == directory.parent_path()) break;
    }
    if (m_EngineRoot.empty()) { m_Status = "Native script build tools are missing from this engine installation"; return; }
    std::filesystem::create_directories(m_Source);
    bool hasSource = false;
    for (const auto& file : std::filesystem::recursive_directory_iterator(m_Source)) if (file.is_regular_file() && file.path().extension() == ".cpp") hasSource = true;
    if (!hasSource) std::filesystem::copy_file(m_EngineRoot / "Scripts/Templates/ProjectScripts.cpp",m_Source / "ProjectScripts.cpp",std::filesystem::copy_options::skip_existing);
    m_Library = m_Root / ".Finality/NativeScripts/bin" / Configuration / "GameScripts.dll";
    m_Observed = Fingerprint(); m_Changed = std::chrono::steady_clock::now(); m_Built = 0;
    m_RequestBuild = project.Settings.AutoBuildScripts;
    m_Status = "Project scripts ready to build";
    if (std::filesystem::is_regular_file(m_Library))
    {
        std::string error;
        if (m_Module.Load(m_Library,m_Root / ".Finality/NativeScripts/Loaded",SceneManager::GetInstance().GetActiveScene(),error)) m_Status = "Project script module loaded";
        else { m_Diagnostics = error; m_Status = "Project scripts need rebuilding"; }
    }
}

void FINALITY::ProjectScripts::Close()
{
    if (m_Build.valid()) { m_Build.wait(); m_Build.get(); }
    m_Module.Unload();
    m_Root.clear(); m_EngineRoot.clear(); m_Source.clear(); m_Library.clear();
    m_RequestBuild = m_PendingReload = false; m_Observed = m_Built = 0;
    m_Diagnostics.clear(); m_Status = "No project scripts loaded";
}

void FINALITY::ProjectScripts::Update(const Project& project, Scene* scene, bool canReload)
{
    if (m_Root.empty() || m_EngineRoot.empty()) return;
    const auto now = std::chrono::steady_clock::now();
    if (now >= m_NextScan)
    {
        m_NextScan = now + std::chrono::milliseconds(250);
        const auto hash = Fingerprint();
        if (hash != m_Observed) { m_Observed = hash; m_Changed = now; }
    }
    if (m_Build.valid() && m_Build.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        const auto result = m_Build.get(); m_Built = m_Building;
        m_Diagnostics = result.Output;
        std::filesystem::create_directories(m_Root / ".Finality/NativeScripts");
        std::ofstream log(m_Root / ".Finality/NativeScripts/Build.log"); log << result.Output;
        if (result.ExitCode == 0) { m_PendingReload = true; m_Status = "Build succeeded; waiting to reload"; }
        else
        {
            m_Status = "Script build failed; the previous module remains active";
            const auto end = m_Diagnostics.size();
            FI_CLIENT_ERROR("{}\n{}",m_Status,m_Diagnostics.substr(end > 3000 ? end - 3000 : 0));
        }
    }
    if (m_PendingReload && canReload)
    {
        m_PendingReload = false;
        std::string error;
        if (m_Module.Load(m_Library,m_Root / ".Finality/NativeScripts/Loaded",scene,error)) m_Status = "Project scripts loaded, generation " + std::to_string(m_Module.GetGeneration());
        else { m_Status = "Reload failed; the previous module remains active"; m_Diagnostics = error; }
    }
    else if (m_PendingReload) m_Status = "Build succeeded; Stop Play mode to apply the new module";
    const bool changed = project.Settings.AutoBuildScripts && m_Observed != m_Built && std::chrono::duration<float>(now - m_Changed).count() >= project.Settings.ScriptBuildDelay;
    if (!m_Build.valid() && (m_RequestBuild || changed))
    {
        m_RequestBuild = false; m_Building = m_Observed; m_Status = "Building project scripts";
        const std::vector<std::wstring> arguments = {L"powershell.exe",L"-NoProfile",L"-NonInteractive",L"-ExecutionPolicy",L"RemoteSigned",L"-File",(m_EngineRoot / "Scripts/NativeModule.ps1").wstring(),
            L"-ProjectRoot",m_Root.wstring(),L"-EngineRoot",m_EngineRoot.wstring(),L"-EngineBin",m_EngineBin.wstring(),
            L"-ScriptDirectory",std::filesystem::path(project.Settings.ScriptDirectory).wstring(),L"-Configuration",Configuration};
        const auto directory = m_Root;
        m_Build = std::async(std::launch::async,[arguments,directory]() { return Process::Run(arguments,directory); });
    }
}
