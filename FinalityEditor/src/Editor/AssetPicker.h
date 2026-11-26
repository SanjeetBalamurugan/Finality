/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for AssetPicker.h.
 */
#pragma once

#include <Finality.h>
#include <functional>
#include <imgui.h>

namespace FINALITY
{
    bool EditText(const char* label, std::string& value);
    bool MatchesSearch(const std::string& text, const std::string& query);
    void DrawAssetPreview(const AssetRecord& asset, float size = 48.0f);
    void DrawAssetPreviewAt(const AssetRecord& asset, ImVec2 position, float size);

    /** @todo TODO: Document AssetPicker class semantics, ownership and usage. */
    class AssetPicker
    {
    public:
        void Open(ProjectAssetType type, const std::string& reference,
                  std::function<void(const std::string&)> assign, const std::string& extension = {});
        void Draw();
        void Close();

    private:
        bool m_RequestOpen = false;
        ProjectAssetType m_Type = ProjectAssetType::Unknown;
        std::string m_Search;
        std::string m_Selected;
        std::string m_Extension;
        std::function<void(const std::string&)> m_Assign;
    };
} // namespace FINALITY
