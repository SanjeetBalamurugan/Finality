#include "EditorPreferences.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace FINALITY
{
    const std::array<ShortcutDefinition, EditorPreferences::Count>& EditorPreferences::Definitions()
    {
        static const std::array<ShortcutDefinition, Count> values{ { { "NewScene", "New Scene", "Global", ImGuiMod_Ctrl | ImGuiKey_N },
                                                                     { "OpenScene", "Open Scene", "Global", ImGuiMod_Ctrl | ImGuiKey_O },
                                                                     { "Save", "Save Scene", "Global", ImGuiMod_Ctrl | ImGuiKey_S },
                                                                     { "SaveAs", "Save Scene As", "Global", ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S },
                                                                     { "Undo", "Undo", "Global", ImGuiMod_Ctrl | ImGuiKey_Z },
                                                                     { "Redo", "Redo", "Global", ImGuiMod_Ctrl | ImGuiKey_Y },
                                                                     { "Copy", "Copy Entity", "Scene / Hierarchy", ImGuiMod_Ctrl | ImGuiKey_C },
                                                                     { "Cut", "Cut Entity", "Scene / Hierarchy", ImGuiMod_Ctrl | ImGuiKey_X },
                                                                     { "Paste", "Paste Entity", "Scene / Hierarchy", ImGuiMod_Ctrl | ImGuiKey_V },
                                                                     { "Duplicate", "Duplicate Selection", "Selection", ImGuiMod_Ctrl | ImGuiKey_D },
                                                                     { "Delete", "Delete Selection", "Selection", ImGuiKey_Delete },
                                                                     { "Rename", "Rename Selection", "Selection", ImGuiKey_F2 },
                                                                     { "Frame", "Frame Selection", "Scene / Hierarchy", ImGuiKey_F },
                                                                     { "Move", "Move Tool", "Scene / Hierarchy", ImGuiKey_W },
                                                                     { "Rotate", "Rotate Tool", "Scene / Hierarchy", ImGuiKey_E },
                                                                     { "Scale", "Scale Tool", "Scene / Hierarchy", ImGuiKey_R },
                                                                     { "CreateEmpty", "Create Empty Entity", "Scene / Hierarchy", ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_N },
                                                                     { "Play", "Play / Stop", "Global", ImGuiKey_F5 },
                                                                     { "Pause", "Pause / Resume", "Global", ImGuiKey_F6 },
                                                                     { "Stop", "Stop", "Global", ImGuiMod_Shift | ImGuiKey_F5 },
                                                                     { "Search", "Focus Search", "Hierarchy / Content Browser", ImGuiMod_Ctrl | ImGuiKey_F },
                                                                     { "OpenAsset", "Open Selected Asset", "Content Browser", ImGuiKey_Enter },
                                                                     { "ParentFolder", "Parent Folder", "Content Browser", ImGuiKey_Backspace } } };
        return values;
    }

    EditorPreferences::EditorPreferences()
    {
        for (size_t i = 0; i < Count; ++i)
        {
            m_Keys[i] = Definitions()[i].Default;
        }
    }

    bool EditorPreferences::Pressed(EditorAction action) const
    {
        const auto key = Get(action);
        return key && ImGui::IsKeyChordPressed(key);
    }

    std::string EditorPreferences::Label(EditorAction action) const
    {
        return ChordName(Get(action));
    }

    std::string EditorPreferences::ChordName(ImGuiKeyChord chord)
    {
        if (!chord)
        {
            return "Unbound";
        }
        std::string text;
        if (chord & ImGuiMod_Ctrl)
        {
            text += "Ctrl+";
        }
        if (chord & ImGuiMod_Shift)
        {
            text += "Shift+";
        }
        if (chord & ImGuiMod_Alt)
        {
            text += "Alt+";
        }
        if (chord & ImGuiMod_Super)
        {
            text += "Super+";
        }
        return text + ImGui::GetKeyName(static_cast<ImGuiKey>(chord & ~ImGuiMod_Mask_));
    }

    bool EditorPreferences::ParseChord(const std::string& text, ImGuiKeyChord& chord)
    {
        std::string cleaned;
        for (unsigned char c : text)
        {
            if (!std::isspace(c))
            {
                cleaned += static_cast<char>(std::tolower(c));
            }
        }
        if (cleaned == "unbound" || cleaned == "none")
        {
            chord = 0;
            return true;
        }
        std::istringstream tokens(cleaned);
        std::string token;
        ImGuiKeyChord result = 0;
        bool found = false;
        while (std::getline(tokens, token, '+'))
        {
            if (token == "ctrl")
            {
                result |= ImGuiMod_Ctrl;
            }
            else if (token == "shift")
            {
                result |= ImGuiMod_Shift;
            }
            else if (token == "alt")
            {
                result |= ImGuiMod_Alt;
            }
            else if (token == "super")
            {
                result |= ImGuiMod_Super;
            }
            else
            {
                if (found)
                {
                    return false;
                }
                for (int value = ImGuiKey_NamedKey_BEGIN; value < ImGuiKey_GamepadStart; ++value)
                {
                    if (value >= ImGuiKey_LeftCtrl && value <= ImGuiKey_RightSuper)
                    {
                        continue;
                    }
                    std::string name = ImGui::GetKeyName(static_cast<ImGuiKey>(value));
                    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c)
                                   {
                                       return static_cast<char>(std::tolower(c));
                                   });
                    if (name == token)
                    {
                        result |= value;
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    return false;
                }
            }
        }
        if (!found)
        {
            return false;
        }
        chord = result;
        return true;
    }

    int EditorPreferences::Conflict(EditorAction action, ImGuiKeyChord key) const
    {
        if (key)
        {
            for (size_t i = 0; i < Count; ++i)
            {
                if (i != static_cast<size_t>(action) && m_Keys[i] == key)
                {
                    return static_cast<int>(i);
                }
            }
        }
        return -1;
    }

    void EditorPreferences::Load(const std::filesystem::path& root)
    {
        std::ifstream stream(root / "Settings/Keymaps.ini");
        std::string line;
        if (!std::getline(stream, line))
        {
            return;
        }
        std::istringstream legacy(line);
        int a = 0, b = 0, c = 0;
        if (legacy >> a >> b >> c)
        {
            auto valid = [](int value)
            {
                return value >= ImGuiKey_NamedKey_BEGIN && value < ImGuiKey_GamepadStart;
            };
            if (valid(a) && valid(b) && valid(c))
            {
                Set(EditorAction::Delete, a);
                Set(EditorAction::Duplicate, ImGuiMod_Ctrl | b);
                Set(EditorAction::Rename, c);
            }
            return;
        }
        do
        {
            const auto equals = line.find('=');
            if (equals == std::string::npos)
            {
                continue;
            }
            auto name = line.substr(0, equals);
            name.erase(std::remove_if(name.begin(), name.end(), [](unsigned char c)
                                      {
                                          return std::isspace(c);
                                      }),
                       name.end());
            for (size_t i = 0; i < Count; ++i)
            {
                if (name == Definitions()[i].Name)
                {
                    ImGuiKeyChord key;
                    if (ParseChord(line.substr(equals + 1), key))
                    {
                        m_Keys[i] = key;
                    }
                    break;
                }
            }
        } while (std::getline(stream, line));
    }

    void EditorPreferences::Save(const std::filesystem::path& root) const
    {
        std::filesystem::create_directories(root / "Settings");
        std::ofstream stream(root / "Settings/Keymaps.ini");
        stream << "[Shortcuts]\n";
        for (size_t i = 0; i < Count; ++i)
        {
            stream << Definitions()[i].Name << " = " << ChordName(m_Keys[i]) << '\n';
        }
    }
} // namespace FINALITY
