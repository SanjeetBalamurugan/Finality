#include "AssetPicker.h"
#include <ImGUI/AssetPreview.h>
#include <algorithm>
#include <array>
#include <cstring>

bool FINALITY::EditText(const char* label, std::string& value)
{
    std::vector<char> buffer(std::max<size_t>(1024, value.size() + 256), 0);
    std::memcpy(buffer.data(), value.data(), value.size());
    if (!ImGui::InputText(label, buffer.data(), buffer.size()))
    {
        return false;
    }
    value = buffer.data();
    return true;
}

bool FINALITY::MatchesSearch(const std::string& text, const std::string& query)
{
    auto lower = [](std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
                       {
                           return static_cast<char>(std::tolower(c));
                       });
        return value;
    };
    return lower(text).find(lower(query)) != std::string::npos;
}

void FINALITY::DrawAssetPreview(const AssetRecord& asset, float size)
{
    DrawAssetPreviewAt(asset, ImGui::GetCursorScreenPos(), size);
    ImGui::Dummy({ size, size });
}

void FINALITY::DrawAssetPreviewAt(const AssetRecord& asset, ImVec2 position, float size)
{
    const ImVec2 end{ position.x + size, position.y + size };
    const bool visible = ImGui::IsRectVisible(position, end);
    if (visible && (asset.Type == ProjectAssetType::Model || asset.Type == ProjectAssetType::Material) && asset.Status == "Ready")
    {
        if (void* handle = AssetPreview::RenderedHandle("asset://" + std::to_string(asset.ID), asset.Type))
        {
            ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(handle)), position, end);
            return;
        }
    }
    if (visible && asset.Type == ProjectAssetType::Texture && asset.Status == "Ready")
    {
        if (void* handle = AssetPreview::TextureHandle("asset://" + std::to_string(asset.ID)))
        {
            ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(handle)), position, end);
            return;
        }
    }
    ImGui::GetWindowDrawList()->AddRectFilled(position, { position.x + size, position.y + size }, ImGui::GetColorU32(ImGuiCol_FrameBg), 5);
    const char* labels[] = { "FILE", "TEX", "3D", "MAT", "GLSL", "SCENE", "C++", "PREFAB" };
    ImGui::GetWindowDrawList()->AddText({ position.x + 3, position.y + size / 3 }, ImGui::GetColorU32(ImGuiCol_TextDisabled), labels[static_cast<int>(asset.Type)]);
}

void FINALITY::AssetPicker::Open(ProjectAssetType type, const std::string& reference,
                                 std::function<void(const std::string&)> assign, const std::string& extension)
{
    m_Type = type;
    m_Selected = reference;
    m_Assign = std::move(assign);
    m_Extension = extension;
    m_Search.clear();
    m_RequestOpen = true;
}

void FINALITY::AssetPicker::Close()
{
    m_RequestOpen = false;
    m_Assign = {};
}

void FINALITY::AssetPicker::Draw()
{
    if (m_RequestOpen)
    {
        ImGui::OpenPopup("Select Finality Asset");
        m_RequestOpen = false;
    }
    ImGui::SetNextWindowSize({ 600, 500 }, ImGuiCond_FirstUseEver);
    bool open = true;
    if (!ImGui::BeginPopupModal("Select Finality Asset", &open))
    {
        return;
    }
    EditText("Search", m_Search);
    ImGui::Separator();
    if (ImGui::BeginChild("Assets", { 0, -70 }))
    {
        if (ImGui::Selectable("None / Clear", m_Selected.empty()))
        {
            m_Selected.clear();
        }
        if (m_Type == ProjectAssetType::Model)
        {
            constexpr const char* primitiveNames[] =
            {
                "cube",
                "sphere",
                "capsule",
                "cylinder",
                "plane",
                "quad"
            };

            for (const char* primitive : primitiveNames)
            {
                const std::string reference = std::string("builtin://") + primitive;
                if (MatchesSearch(primitive, m_Search) && ImGui::Selectable(primitive, m_Selected == reference))
                {
                    m_Selected = reference;
                }
            }
        }
        for (const auto& [id, asset] : AssetRegistry::GetRecords())
        {
            if (asset.Type != m_Type || asset.Status != "Ready" || !MatchesSearch(asset.Path, m_Search))
            {
                continue;
            }
            if (!m_Extension.empty() && std::filesystem::path(asset.Path).extension() != m_Extension)
            {
                continue;
            }
            ImGui::PushID(static_cast<int>(id));
            DrawAssetPreview(asset);
            ImGui::SameLine();
            const std::string reference = "asset://" + std::to_string(id);
            if (ImGui::Selectable(asset.Path.c_str(), m_Selected == reference, ImGuiSelectableFlags_AllowDoubleClick, { 0, 48 }))
            {
                m_Selected = reference;
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    ImGui::TextWrapped("Selected: %s", m_Selected.empty() ? "None" : m_Selected.c_str());
    if (ImGui::Button("Confirm"))
    {
        if (m_Selected.empty() || m_Selected.starts_with("builtin://") || std::filesystem::is_regular_file(AssetRegistry::Resolve(m_Selected)))
        {
            if (m_Assign)
            {
                m_Assign(m_Selected);
            }
        }
        m_Assign = {};
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel") || !open)
    {
        m_Assign = {};
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}
