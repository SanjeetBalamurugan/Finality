#pragma once

#include "ScriptRegistry.h"
#include "ScriptReflection.h"
#include <filesystem>

namespace FINALITY
{
    inline constexpr uint64_t ScriptModuleABI()
    {
        uint64_t value = 0x46494e0100000000ull | (sizeof(ScriptValue) << 16) | sizeof(ScriptDefinition);
#ifdef _MSC_VER
        value ^= static_cast<uint64_t>(_MSC_VER) << 24;
#endif
#ifdef _ITERATOR_DEBUG_LEVEL
        value ^= static_cast<uint64_t>(_ITERATOR_DEBUG_LEVEL) << 40;
#endif
        return value;
    }

    struct ScriptRegistrationContext
    {
        void* User = nullptr;
        bool (*Register)(void*,const ScriptDefinition&) = nullptr;
        bool Add(const ScriptDefinition& definition) const { return Register && Register(User,definition); }
    };

    class FAPI ScriptModule
    {
    public:
        ScriptModule();
        ~ScriptModule();
        ScriptModule(const ScriptModule&) = delete;
        ScriptModule& operator=(const ScriptModule&) = delete;
        bool Load(const std::filesystem::path& library, const std::filesystem::path& shadowDirectory, Scene* scene, std::string& error);
        void Unload();
        bool IsLoaded() const;
        uint64_t GetGeneration() const;

    private:
        struct State;
        std::unique_ptr<State> m_State;
    };
}
