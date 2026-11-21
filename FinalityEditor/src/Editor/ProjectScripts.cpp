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
    std::vector<std::filesystem::path> inputs;
    for (std::filesystem::recursive_directory_iterator it(m_Source,std::filesystem::directory_options::skip_permission_denied,error),end; it != end && !error; it.increment(error))
    {
        if (!it->is_regular_file()) continue;
        const auto extension = it->path().extension();
        if (extension != ".cpp" && extension != ".h" && extension != ".hpp" && extension != ".inl") continue;
        inputs.push_back(it->path());
    }
    for (const auto& input : {m_EngineBin / "FinalityEngine.dll", m_EngineBin / "FinalityBuild.json", m_EngineBin / "FIBuild.exe", m_EngineBin / "FIScriptReflect.exe", m_EngineBin / "libclang.dll", m_EngineRoot / "Scripts/Templates/ProjectScripts.lua"})
        if (std::filesystem::is_regular_file(input)) inputs.push_back(input);
    for (const auto& helper : {"Scripts/NoVisualStudioFilters.lua"})
        if (std::filesystem::is_regular_file(m_EngineRoot / helper)) inputs.push_back(m_EngineRoot / helper);
    std::sort(inputs.begin(),inputs.end());
    for (const auto& input : inputs)
    {
        const auto key = input.string() + std::to_string(std::filesystem::last_write_time(input).time_since_epoch().count()) + std::to_string(std::filesystem::file_size(input));
        for (unsigned char c : key) { value ^= c; value *= 1099511628211ull; }
    }
    return value;
}

void FINALITY::ProjectScripts::Open(const Project& project, const std::filesystem::path& engineAssets)
{
    Close();
    m_Root = project.GetDirectory(); m_Source = m_Root / project.Settings.ScriptDirectory;
    const auto assets = m_Root / "Assets";
    const auto relative = m_Source.lexically_relative(assets);
    if (!relative.empty() && *relative.begin() != "..") m_Source = assets;
    m_EngineBin = Process::ExecutablePath().parent_path();
    for (auto directory = engineAssets; !directory.empty(); directory = directory.parent_path())
    {
        if (std::filesystem::is_regular_file(directory / "Scripts/Templates/ProjectScripts.lua")) { m_EngineRoot = directory; break; }
        if (directory == directory.parent_path()) break;
    }
    if (m_EngineRoot.empty()) { m_Status = "Native script build tools are missing from this engine installation"; return; }
    std::filesystem::create_directories(m_Source);
    m_Library = m_Root / ".Finality/NativeScripts/bin" / Configuration / "GameScripts.dll";
    m_Observed = Fingerprint(); m_Changed = std::chrono::steady_clock::now(); m_Built = 0;
    std::ifstream(m_Root / (std::string(".Finality/NativeScripts/") + (Configuration == std::wstring(L"Release") ? "Release" : "Debug") + ".fingerprint")) >> m_Built;
    m_Attempted = m_Built;
    m_RequestBuild = project.Settings.AutoBuildScripts && m_Built != m_Observed;
    if (!std::filesystem::is_regular_file(m_Library)) { m_Built = 0; m_RequestBuild = project.Settings.AutoBuildScripts; }
    m_Status = "Project scripts ready to build";
    if (std::filesystem::is_regular_file(m_Library))
    {
        std::string error;
        if (m_Module.Load(m_Library,m_Root / ".Finality/NativeScripts/Loaded",SceneManager::GetInstance().GetActiveScene(),error)) m_Status = "Project script module loaded";
        else { m_Diagnostics = error; m_Status = "Project scripts need rebuilding"; m_Attempted = m_Built = 0; m_RequestBuild = project.Settings.AutoBuildScripts; }
    }
}

void FINALITY::ProjectScripts::Close()
{
    if (m_Build.valid()) { m_Build.wait(); m_Build.get(); }
    m_Module.Unload();
    m_Root.clear(); m_EngineRoot.clear(); m_Source.clear(); m_Library.clear();
    m_RequestBuild = m_PendingReload = false; m_Observed = m_Built = m_Attempted = 0; m_NextScan = {};
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
        const auto result = m_Build.get(); m_Attempted = m_Building;
        m_Diagnostics = result.Output;
        std::filesystem::create_directories(m_Root / ".Finality/NativeScripts");
        std::ofstream log(m_Root / ".Finality/NativeScripts/Build.log"); log << result.Output;
        if (result.ExitCode == 0)
        {
            m_PendingReload = true; m_Status = "Build succeeded; waiting to reload";
        }
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
        if (m_Module.Load(m_Library,m_Root / ".Finality/NativeScripts/Loaded",scene,error))
        {
            m_Built = m_Attempted;
            std::ofstream(m_Root / (std::string(".Finality/NativeScripts/") + (Configuration == std::wstring(L"Release") ? "Release" : "Debug") + ".fingerprint")) << m_Built;
            m_Status = "Project scripts loaded, generation " + std::to_string(m_Module.GetGeneration());
        }
        else { m_Status = "Reload failed; the previous module remains active"; m_Diagnostics = error; }
    }
    else if (m_PendingReload) m_Status = "Build succeeded; Stop Play mode to apply the new module";
    const bool changed = project.Settings.AutoBuildScripts && m_Observed != m_Attempted && std::chrono::duration<float>(now - m_Changed).count() >= project.Settings.ScriptBuildDelay;
    if (!m_Build.valid() && (m_RequestBuild || changed))
    {
        m_RequestBuild = false; m_Building = m_Observed; m_Status = "Building project scripts";
        const std::vector<std::wstring> arguments = {(m_EngineBin / "FIBuild.exe").wstring(),L"native",
            L"--project",m_Root.wstring(),L"--name",std::filesystem::path(project.GetName()).wstring(),L"--engine",m_EngineRoot.wstring(),L"--bin",m_EngineBin.wstring(),
            L"--source",std::filesystem::path(project.Settings.ScriptDirectory).wstring(),L"--configuration",Configuration};
        const auto directory = m_Root;
        m_Build = std::async(std::launch::async,[arguments,directory]() { return Process::Run(arguments,directory); });
    }
}
