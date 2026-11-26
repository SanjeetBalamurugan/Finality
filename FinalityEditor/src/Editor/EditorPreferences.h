/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for EditorPreferences.h.
 */
#pragma once
#include <imgui.h>
#include <array>
#include <filesystem>
#include <string>

namespace FINALITY
{
    /** @todo TODO: Document EditorAction enum semantics, ownership and usage. */
    enum class EditorAction
    {
        NewScene,
        OpenScene,
        Save,
        SaveAs,
        Undo,
        Redo,
        Copy,
        Cut,
        Paste,
        Duplicate,
        Delete,
        Rename,
        Frame,
        Move,
        Rotate,
        Scale,
        CreateEmpty,
        Play,
        Pause,
        Stop,
        Search,
        OpenAsset,
        ParentFolder,
        Count
    };

    /** @todo TODO: Document ShortcutDefinition struct semantics, ownership and usage. */
    struct ShortcutDefinition
    {
        const char* Name;
        const char* Label;
        const char* Scope;
        ImGuiKeyChord Default;
    };

    /** @todo TODO: Document EditorPreferences class semantics, ownership and usage. */
    class EditorPreferences
    {
    public:
        static constexpr size_t Count = static_cast<size_t>(EditorAction::Count);
        static const std::array<ShortcutDefinition, Count>& Definitions();
        EditorPreferences();

        ImGuiKeyChord Get(EditorAction action) const
        {
            return m_Keys[static_cast<size_t>(action)];
        }

        void Set(EditorAction action, ImGuiKeyChord key)
        {
            m_Keys[static_cast<size_t>(action)] = key;
        }

        bool Pressed(EditorAction action) const;
        std::string Label(EditorAction action) const;
        static std::string ChordName(ImGuiKeyChord key);
        static bool ParseChord(const std::string& text, ImGuiKeyChord& key);
        int Conflict(EditorAction action, ImGuiKeyChord key) const;
        void Load(const std::filesystem::path& root);
        void Save(const std::filesystem::path& root) const;

    private:
        std::array<ImGuiKeyChord, Count> m_Keys;
    };
} // namespace FINALITY
