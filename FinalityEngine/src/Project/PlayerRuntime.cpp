#include "PlayerRuntime.h"
#include <bit>
#include <cmath>
#include <filesystem>
#include <stdexcept>

namespace
{
    void Validate(const FINALITY::PlayerRuntime& value)
    {
        if (value.Width < 320 || value.Width > 16384 || value.Height < 200 || value.Height > 16384 || !std::isfinite(value.Models.Scale) || value.Models.Scale <= 0 || value.Scenes.empty() || value.Scenes.size() > 4096)
        {
            throw std::runtime_error("Invalid embedded player settings");
        }
        for (const auto& scene : value.Scenes)
        {
            const std::filesystem::path path(scene);
            if (scene.size() > 8192 || scene.find('\0') != std::string::npos || path.is_absolute() || path.lexically_normal().empty() || *path.lexically_normal().begin() != "Assets" || path.extension() != ".fscene")
            {
                throw std::runtime_error("Invalid embedded scene path");
            }
            for (const auto& part : path)
            {
                if (part == "..")
                {
                    throw std::runtime_error("Scene path escapes game data");
                }
            }
        }
    }
} // namespace

std::vector<uint8_t> FINALITY::EncodePlayerRuntime(const PlayerRuntime& settings)
{
    Validate(settings);
    std::vector<uint8_t> bytes{ 'F', 'I', 'P', 'L', 'A', 'Y', '0', '1' };
    auto word = [&](uint32_t value)
    {
        for (int shift = 0; shift < 32; shift += 8)
        {
            bytes.push_back(uint8_t(value >> shift));
        }
    };
    word(settings.Width);
    word(settings.Height);
    word(settings.Fullscreen ? 1 : 0);
    word(std::bit_cast<uint32_t>(settings.Models.Scale));
    word((settings.Models.ConvertUnits ? 1 : 0) | (settings.Models.GenerateNormals ? 2 : 0) |
         (settings.Models.FlipUVs ? 4 : 0) | (settings.Models.ImportMaterials ? 8 : 0));
    word(static_cast<uint32_t>(settings.Scenes.size()));
    for (const auto& scene : settings.Scenes)
    {
        word(static_cast<uint32_t>(scene.size()));
        bytes.insert(bytes.end(), scene.begin(), scene.end());
    }
    if (bytes.size() > 1024 * 1024)
    {
        throw std::runtime_error("Embedded player data exceeds 1 MiB");
    }
    return bytes;
}

FINALITY::PlayerRuntime FINALITY::DecodePlayerRuntime(std::span<const uint8_t> bytes)
{
    if (bytes.size() < 32 || bytes.size() > 1024 * 1024 || std::string(bytes.begin(), bytes.begin() + 8) != "FIPLAY01")
    {
        throw std::runtime_error("Missing or unsupported embedded player data; rebuild the game with FIBaker");
    }
    size_t cursor = 8;
    auto word = [&]()
    {
        if (cursor + 4 > bytes.size())
        {
            throw std::runtime_error("Truncated player data");
        }
        uint32_t value = 0;
        for (int shift = 0; shift < 32; shift += 8)
        {
            value |= uint32_t(bytes[cursor++]) << shift;
        }
        return value;
    };
    PlayerRuntime settings;
    settings.Width = static_cast<int>(word());
    settings.Height = static_cast<int>(word());
    const auto fullscreen = word();
    if (fullscreen > 1)
    {
        throw std::runtime_error("Invalid fullscreen flag");
    }
    settings.Fullscreen = fullscreen != 0;
    settings.Models.Scale = std::bit_cast<float>(word());
    const auto flags = word();
    if (flags & ~15u)
    {
        throw std::runtime_error("Invalid model flags");
    }
    settings.Models.ConvertUnits = (flags & 1) != 0;
    settings.Models.GenerateNormals = (flags & 2) != 0;
    settings.Models.FlipUVs = (flags & 4) != 0;
    settings.Models.ImportMaterials = (flags & 8) != 0;
    const auto count = word();
    if (count > 4096)
    {
        throw std::runtime_error("Too many embedded scenes");
    }
    for (uint32_t index = 0; index < count; ++index)
    {
        const auto length = word();
        if (length > 8192 || length > bytes.size() - cursor)
        {
            throw std::runtime_error("Truncated scene path");
        }
        settings.Scenes.emplace_back(reinterpret_cast<const char*>(bytes.data() + cursor), length);
        cursor += length;
    }
    if (cursor != bytes.size())
    {
        throw std::runtime_error("Unexpected trailing player data");
    }
    Validate(settings);
    return settings;
}
