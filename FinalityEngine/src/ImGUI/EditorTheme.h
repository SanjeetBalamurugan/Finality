#pragma once

#include <Core/Core.h>
#include <filesystem>
#include <imgui.h>
#include <string>
#include <vector>

namespace FINALITY
{
struct ThemeChoice
{
    std::string ID;
    std::string Name;
};

class FAPI EditorTheme
{
  public:
    static std::vector<ThemeChoice> List(const std::filesystem::path &engineAssets,
                                         const std::filesystem::path &project);
    static bool Load(const std::string &id, const std::filesystem::path &engineAssets,
                     const std::filesystem::path &project, ImGuiStyle &style, std::string &error);
    static bool Apply(const std::string &id, const std::filesystem::path &engineAssets,
                      const std::filesystem::path &project, std::string &error);
    static bool Export(const std::filesystem::path &path, const std::string &name, std::string &error);
};
}
