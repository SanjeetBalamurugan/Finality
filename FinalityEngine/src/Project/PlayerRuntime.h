#pragma once
#include <Core/Core.h>
#include <Assets/ModelImportOptions.h>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace FINALITY
{
    /** @brief Only the settings consumed by a running game; embedded by the baker, independent of editor YAML. */
    struct PlayerRuntime
    {
        int Width = 1280, Height = 720;
        bool Fullscreen = false;
        ModelImportOptions Models;
        std::vector<std::string> Scenes;
    };

    inline constexpr int PlayerRuntimeResource = 101;
    FAPI std::vector<uint8_t> EncodePlayerRuntime(const PlayerRuntime& settings);
    FAPI PlayerRuntime DecodePlayerRuntime(std::span<const uint8_t> bytes);
} // namespace FINALITY
