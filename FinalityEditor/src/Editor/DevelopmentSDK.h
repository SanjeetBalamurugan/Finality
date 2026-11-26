#pragma once
#include <filesystem>

namespace FINALITY
{
    /** @brief Prefer the installed development SDK; source builds may fall back to their checkout. */
    inline std::filesystem::path FindDevelopmentRoot(const std::filesystem::path& bin, const std::filesystem::path& hint = {})
    {
        namespace fs = std::filesystem;
        if (fs::is_regular_file(bin / "SDK/FinalitySDK.json") && fs::is_regular_file(bin / "SDK/Scripts/Templates/ProjectScripts.lua"))
        {
            return bin / "SDK";
        }
        const std::filesystem::path searchRoots[] =
        {
            hint,
            bin
        };

        for (const auto& start : searchRoots)
        {
            for (auto current = start; !current.empty(); current = current.parent_path())
            {
                if (fs::is_regular_file(current / "Scripts/Templates/ProjectScripts.lua"))
                {
                    return current;
                }
                if (current == current.parent_path())
                {
                    break;
                }
            }
        }
        return {};
    }
} // namespace FINALITY
