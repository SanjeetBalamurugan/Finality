/**
 * @file
 * @ingroup scene
 * @brief Loads and replaces native script libraries with ABI validation.
 *
 * The module loads a shadow copy, validates its ABI and registers definitions against the engine. Definitions and instances retain a module lease so native code stays mapped while it is still referenced.
 *
 * @par Usage
 * Build GameScripts.dll against the matching engine toolchain and headers. Load reports diagnostics through its error argument. A failed reload must not discard the previous working implementation.
 */
#pragma once

#include "ScriptRegistry.h"
#include "ScriptReflection.h"
#include <filesystem>

namespace FINALITY
{
    inline constexpr uint64_t ScriptModuleABI()
    {
        uint64_t value = 0x46494e0600000000ull | (sizeof(ScriptValue) << 16) | sizeof(ScriptDefinition);
#ifdef _MSC_VER
        value ^= static_cast<uint64_t>(_MSC_VER) << 24;
#endif
#ifdef _ITERATOR_DEBUG_LEVEL
        value ^= static_cast<uint64_t>(_ITERATOR_DEBUG_LEVEL) << 40;
#endif
        return value;
    }

    /** @brief Module registration callback and caller-owned context passed across the native library boundary. */
    struct ScriptRegistrationContext
    {
        void* User = nullptr;
        bool (*Register)(void*, const ScriptDefinition&) = nullptr;

        bool Add(const ScriptDefinition& definition) const
        {
            return Register && Register(User, definition);
        }
    };

    /**
     * @ingroup scene
     * @brief Loads and replaces native script libraries with ABI validation.
     *
     * The module loads a shadow copy, validates its ABI and registers definitions against the engine. Definitions and instances retain a module lease so native code stays mapped while it is still referenced.
     *
     * @par Usage
     * Build GameScripts.dll against the matching engine toolchain and headers. Load reports diagnostics through its error argument. A failed reload must not discard the previous working implementation.
     */
    class FAPI ScriptModule
    {
    public:
        ScriptModule();
        ~ScriptModule();
        ScriptModule(const ScriptModule&) = delete;
        ScriptModule& operator=(const ScriptModule&) = delete;
        /**
         * @brief Validates and loads a compiled native script module.
         * @param library Compiled module built against the matching engine ABI.
         * @param shadowDirectory Writable directory for unique loaded library copies.
         * @param scene Scene whose attached instances participate in replacement; may be null.
         * @param error Receives a diagnostic when loading or registration fails.
         * @return True when the new module is accepted.
         * Definitions and instances retain module leases so replacement does not unload
         * code while surviving objects still need it. Failed loading preserves the prior module.
         */
        bool Load(const std::filesystem::path& library, const std::filesystem::path& shadowDirectory, Scene* scene, std::string& error);
        void Unload();
        bool IsLoaded() const;
        uint64_t GetGeneration() const;

    private:
        struct State;
        std::unique_ptr<State> m_State;
    };
} // namespace FINALITY
