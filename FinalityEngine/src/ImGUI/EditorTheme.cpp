#include "EditorTheme.h"
#include "Themes/Catppuccin/CatppuccinFrappe.h"
#include "Themes/Catppuccin/CatppuccinLatte.h"
#include "Themes/Catppuccin/CatppuccinMacchiato.h"
#include "Themes/Catppuccin/CatppuccinMocha.h"
#include "Themes/Dark01.h"
#include "Themes/Light01.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <toml.hpp>

namespace
{
    struct Preset
    {
        const char* Name;
        void (*Apply)(ImGuiStyle&);
    };

    const Preset Presets[] = { { "Dark01", FINALITY::ApplyDark01 },
                               { "Light01", FINALITY::ApplyLight01 },
                               { "Catppuccin Mocha", FINALITY::ApplyCatppuccinMocha },
                               { "Catppuccin Macchiato", FINALITY::ApplyCatppuccinMacchiato },
                               { "Catppuccin Frappe", FINALITY::ApplyCatppuccinFrappe },
                               { "Catppuccin Latte", FINALITY::ApplyCatppuccinLatte } };

    float ToLinear(float value)
    {
        return value <= 0.04045f ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f);
    }

    float ToSRGB(float value)
    {
        return value <= 0.0031308f ? value * 12.92f : 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
    }

    void ApplyPreset(const std::string& name, ImGuiStyle& style)
    {
        const auto preset =
            std::find_if(std::begin(Presets), std::end(Presets), [&](const auto& item)
                         {
                             return name == item.Name;
                         });
        if (preset == std::end(Presets))
        {
            throw std::runtime_error("Unknown base theme: " + name);
        }
        ImGui::StyleColorsDark(&style);
        preset->Apply(style);
        style.FramePadding = { 5, 3 };
        style.ItemSpacing = { 6, 4 };
        style.CellPadding = { 5, 2 };
        style.FrameBorderSize = 1;
        for (auto& color : style.Colors)
        {
            color.x = ToLinear(color.x);
            color.y = ToLinear(color.y);
            color.z = ToLinear(color.z);
        }
    }

    ImVec4 ParseColor(const std::string& value)
    {
        if ((value.size() != 7 && value.size() != 9) || value[0] != '#' || value.find_first_not_of("0123456789abcdefABCDEF", 1) != std::string::npos)
        {
            throw std::runtime_error("Colors must use #RRGGBB or #RRGGBBAA");
        }
        ImVec4 result(0, 0, 0, 1);
        for (size_t channel = 0; channel < (value.size() - 1) / 2; ++channel)
        {
            const float number = static_cast<float>(std::stoul(value.substr(1 + channel * 2, 2), nullptr, 16)) / 255;
            (&result.x)[channel] = channel == 3 ? number : ToLinear(number);
        }
        return result;
    }

    void ParseStyle(const toml::table& table, ImGuiStyle& style)
    {
        const std::pair<const char*, float*> numbers[] =
        {
            { "FrameRounding", &style.FrameRounding }, { "WindowRounding", &style.WindowRounding }, { "FrameBorderSize", &style.FrameBorderSize }, { "WindowBorderSize", &style.WindowBorderSize }, { "ScrollbarSize", &style.ScrollbarSize }, { "GrabRounding", &style.GrabRounding }, { "TabRounding", &style.TabRounding }
        };
        const std::pair<const char*, ImVec2*> vectors[] = { { "WindowPadding", &style.WindowPadding },
                                                            { "FramePadding", &style.FramePadding },
                                                            { "ItemSpacing", &style.ItemSpacing },
                                                            { "ItemInnerSpacing", &style.ItemInnerSpacing },
                                                            { "CellPadding", &style.CellPadding } };
        for (const auto& [key, node] : table)
        {
            bool found = false;
            for (const auto& [name, target] : numbers)
            {
                if (key.str() == name)
                {
                    const auto value = node.value<double>();
                    if (!value || !std::isfinite(*value) || *value < 0 || *value > 64)
                    {
                        throw std::runtime_error("Invalid style value: " + std::string(name));
                    }
                    *target = static_cast<float>(*value);
                    found = true;
                }
            }
            for (const auto& [name, target] : vectors)
            {
                if (key.str() == name)
                {
                    const auto* values = node.as_array();
                    if (!values || values->size() != 2)
                    {
                        throw std::runtime_error("Expected two numbers: " + std::string(name));
                    }
                    for (int index = 0; index < 2; ++index)
                    {
                        const auto value = (*values)[index].value<double>();
                        if (!value || !std::isfinite(*value) || *value < 0 || *value > 64)
                        {
                            throw std::runtime_error("Invalid spacing: " + std::string(name));
                        }
                        (&target->x)[index] = static_cast<float>(*value);
                    }
                    found = true;
                }
            }
            if (!found)
            {
                throw std::runtime_error("Unknown style key: " + std::string(key.str()));
            }
        }
    }
} // namespace

std::vector<FINALITY::ThemeChoice> FINALITY::EditorTheme::List(const std::filesystem::path& engineAssets,
                                                               const std::filesystem::path& project)
{
    std::vector<ThemeChoice> choices;
    for (const auto& preset : Presets)
    {
        choices.push_back({ preset.Name, preset.Name });
    }
    const std::pair<std::string, std::filesystem::path> directories[] = { { "bundled:", engineAssets / "themes" },
                                                                          { "project:", project / "Settings/Themes" } };
    for (const auto& [prefix, directory] : directories)
    {
        std::error_code error;
        for (std::filesystem::directory_iterator it(directory, error), end; it != end && !error; it.increment(error))
        {
            if (it->is_regular_file() && it->path().extension() == ".toml")
            {
                choices.push_back({ prefix + it->path().filename().string(), it->path().stem().string() + " (TOML)" });
            }
        }
    }
    std::sort(choices.begin() + std::size(Presets), choices.end(),
              [](const auto& a, const auto& b)
              {
                  return a.Name < b.Name;
              });
    return choices;
}

bool FINALITY::EditorTheme::Load(const std::string& id, const std::filesystem::path& engineAssets,
                                 const std::filesystem::path& project, ImGuiStyle& style, std::string& error)
{
    try
    {
        ImGuiStyle candidate;
        const bool bundled = id.starts_with("bundled:");
        const bool custom = id.starts_with("project:");
        if (!bundled && !custom)
        {
            ApplyPreset(id, candidate);
        }
        else
        {
            const std::filesystem::path name(id.substr(8));
            if (name.filename() != name || name.extension() != ".toml")
            {
                throw std::runtime_error("Invalid theme filename");
            }
            const auto path = (bundled ? engineAssets / "themes" : project / "Settings/Themes") / name;
            const auto data = toml::parse_file(path.string());
            for (const auto& [key, node] : data)
            {
                if (key != "theme" && key != "colors" && key != "style")
                {
                    throw std::runtime_error("Unknown theme section");
                }
            }
            if (data["theme"]["version"].value<int>() != 1)
            {
                throw std::runtime_error("Expected theme version 1");
            }
            if (data.contains("colors") && !data["colors"].is_table())
            {
                throw std::runtime_error("Expected a colors table");
            }
            if (data.contains("style") && !data["style"].is_table())
            {
                throw std::runtime_error("Expected a style table");
            }
            ApplyPreset(data["theme"]["base"].value_or(std::string("Dark01")), candidate);
            if (const auto* colors = data["colors"].as_table())
            {
                for (const auto& [name, node] : *colors)
                {
                    int slot = 0;
                    while (slot < ImGuiCol_COUNT && name.str() != ImGui::GetStyleColorName(slot))
                    {
                        ++slot;
                    }
                    if (slot == ImGuiCol_COUNT)
                    {
                        throw std::runtime_error("Unknown color: " + std::string(name.str()));
                    }
                    const auto value = node.value<std::string>();
                    if (!value)
                    {
                        throw std::runtime_error("Expected a hexadecimal color");
                    }
                    candidate.Colors[slot] = ParseColor(*value);
                }
            }
            if (const auto* values = data["style"].as_table())
            {
                ParseStyle(*values, candidate);
            }
        }
        candidate.FontSizeBase = ImGui::GetStyle().FontSizeBase;
        candidate.FontScaleDpi = ImGui::GetStyle().FontScaleDpi;
        style = candidate;
        error.clear();
        return true;
    }
    catch (const std::exception& failure)
    {
        error = failure.what();
        return false;
    }
}

bool FINALITY::EditorTheme::Apply(const std::string& id, const std::filesystem::path& engineAssets,
                                  const std::filesystem::path& project, std::string& error)
{
    ImGuiStyle style;
    if (!Load(id, engineAssets, project, style, error))
    {
        return false;
    }
    ImGui::GetStyle() = style;
    return true;
}

bool FINALITY::EditorTheme::Export(const std::filesystem::path& path, const std::string& name, std::string& error)
{
    try
    {
        if (std::filesystem::exists(path))
        {
            throw std::runtime_error("Theme file already exists");
        }
        toml::table colors, spacing;
        for (int index = 0; index < ImGuiCol_COUNT; ++index)
        {
            const auto& value = ImGui::GetStyle().Colors[index];
            std::ostringstream hex;
            hex << '#' << std::hex << std::setfill('0');
            for (int channel = 0; channel < 4; ++channel)
            {
                const float component = channel == 3 ? value.w : ToSRGB((&value.x)[channel]);
                hex << std::setw(2) << static_cast<int>(std::round(std::clamp(component, 0.0f, 1.0f) * 255));
            }
            colors.insert(ImGui::GetStyleColorName(index), hex.str());
        }
        const auto& style = ImGui::GetStyle();
        spacing.insert("FrameRounding", style.FrameRounding);
        spacing.insert("FrameBorderSize", style.FrameBorderSize);
        spacing.insert("FramePadding", toml::array{ style.FramePadding.x, style.FramePadding.y });
        spacing.insert("ItemSpacing", toml::array{ style.ItemSpacing.x, style.ItemSpacing.y });
        spacing.insert("CellPadding", toml::array{ style.CellPadding.x, style.CellPadding.y });
        toml::table document{ { "theme", toml::table{ { "version", 1 }, { "name", name }, { "base", "Dark01" } } },
                              { "colors", std::move(colors) },
                              { "style", std::move(spacing) } };
        std::filesystem::create_directories(path.parent_path());
        std::ofstream stream(path);
        stream << document;
        if (!stream)
        {
            throw std::runtime_error("Could not write theme file");
        }
        error.clear();
        return true;
    }
    catch (const std::exception& failure)
    {
        error = failure.what();
        return false;
    }
}
