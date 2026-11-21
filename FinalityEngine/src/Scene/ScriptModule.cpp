#include "ScriptModule.h"
#include <Core/Log.h>
#include <atomic>
#include <chrono>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace
{
    std::atomic<uint64_t> s_Generation{0};
    struct Library
    {
        void* Handle = nullptr;
        std::filesystem::path Path;
        ~Library()
        {
            if (Handle)
            {
#ifdef _WIN32
                FreeLibrary(static_cast<HMODULE>(Handle));
#else
                dlclose(Handle);
#endif
            }
            std::error_code ignored;
            if (!Path.empty()) std::filesystem::remove(Path,ignored);
        }
        void* Symbol(const char* name) const
        {
#ifdef _WIN32
            return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(Handle),name));
#else
            return dlsym(Handle,name);
#endif
        }
    };
    struct Registration
    {
        std::shared_ptr<Library> Lease;
        std::vector<FINALITY::ScriptDefinition> Definitions;
    };
}

struct FINALITY::ScriptModule::State
{
    std::shared_ptr<Library> Lease;
    uint64_t Generation = 0;
};

FINALITY::ScriptModule::ScriptModule() : m_State(std::make_unique<State>()) {}
FINALITY::ScriptModule::~ScriptModule() { Unload(); }
bool FINALITY::ScriptModule::IsLoaded() const { return m_State->Lease != nullptr; }
uint64_t FINALITY::ScriptModule::GetGeneration() const { return m_State->Generation; }
void FINALITY::ScriptModule::Unload()
{
    if (m_State->Lease) ScriptRegistry::ClearModule();
    m_State->Lease.reset();
    m_State->Generation = 0;
}

bool FINALITY::ScriptModule::Load(const std::filesystem::path& library, const std::filesystem::path& shadowDirectory, Scene* scene, std::string& error)
{
    try
    {
        if (!std::filesystem::is_regular_file(library)) throw std::runtime_error("Compiled script library was not found");
        std::filesystem::create_directories(shadowDirectory);
        const auto generation = ++s_Generation;
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto candidate = std::make_shared<Library>();
        const auto shadow = std::filesystem::absolute(shadowDirectory / ("Scripts_" + std::to_string(stamp) + "_" + std::to_string(generation) + library.extension().string()));
        std::filesystem::copy_file(library,shadow);
        candidate->Path = shadow;
#ifdef _WIN32
        candidate->Handle = LoadLibraryExW(candidate->Path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
#else
        candidate->Handle = dlopen(candidate->Path.c_str(),RTLD_NOW | RTLD_LOCAL);
#endif
        if (!candidate->Handle) throw std::runtime_error("Cannot load script library or one of its dependencies");
        const auto abi = reinterpret_cast<uint64_t(*)()>(candidate->Symbol("FinalityScriptABI"));
        const auto registerTypes = reinterpret_cast<bool(*)(ScriptRegistrationContext*)>(candidate->Symbol("FinalityRegisterScripts"));
        if (!abi || !registerTypes || abi() != ScriptModuleABI()) throw std::runtime_error("Script module ABI does not match this engine build");
        Registration registration{candidate,{}};
        ScriptRegistrationContext context;
        context.User = &registration;
        context.Register = [](void* user, const ScriptDefinition& definition)
        {
            auto& result = *static_cast<Registration*>(user);
            auto copy = definition; copy.ModuleLease = result.Lease;
            result.Definitions.push_back(std::move(copy));
            return true;
        };
        if (!registerTypes(&context)) throw std::runtime_error("Script module registration failed");
        if (!ScriptRegistry::ReplaceModule(registration.Definitions,candidate,scene,error)) return false;
        m_State->Lease = std::move(candidate); m_State->Generation = generation;
        FI_CORE_INFO("Loaded project scripts, generation {}",generation);
        return true;
    }
    catch (const std::exception& failure) { error = failure.what(); FI_CORE_ERROR("Script reload failed: {}",error); return false; }
}
