#include "ModelImportPanel.h"
#include <Editor/ExternalTools.h>
#include <ImGUI/AssetPreview.h>
#include <Core/FileDialogs.h>
#include <Assets/AssetRegistry.h>
#include <imgui.h>

void FINALITY::ModelImportPanel::Draw(const std::string& reference, ExternalTools& tools)
{
    if (m_Reference != reference) { m_Reference = reference; m_Draft = ModelAsset::GetImportOptions(reference); m_Message.clear(); }
    if (ImGui::CollapsingHeader("Model Import Settings",ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat("Scale Factor",&m_Draft.Scale,0.01f,0.0001f,10000,"%.4g",ImGuiSliderFlags_AlwaysClamp);
        ImGui::Checkbox("Convert FBX file units to metres",&m_Draft.ConvertUnits);
        ImGui::Checkbox("Generate Missing Normals",&m_Draft.GenerateNormals);
        ImGui::Checkbox("Flip UVs",&m_Draft.FlipUVs);
        ImGui::Checkbox("Assign Imported Materials",&m_Draft.ImportMaterials);
        if (ImGui::Button("Apply and Reimport"))
        {
            if (ModelAsset::SaveImportOptions(reference,m_Draft,m_Message))
            {
                const auto model = ModelAsset::Load(reference,true);
                AssetRegistry::Scan(); AssetPreview::Clear();
                m_Message = "Reimported. New instances use these settings; re-add existing instances to update their hierarchy scale.";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Revert")) { m_Draft = ModelAsset::GetImportOptions(reference); m_Message.clear(); }
        ImGui::TextWrapped("One engine unit is one metre. Scale Factor multiplies the file's authored units; it does not resize the mesh to fit the camera.");
    }
    if (ImGui::Button("Show Imported Materials"))
    {
        const auto model = ModelAsset::Load(reference);
        if (!model->Materials.empty() && !model->Materials.front().Asset.empty())
            ShellExecuteW(nullptr,L"open",std::filesystem::path(AssetRegistry::Resolve(model->Materials.front().Asset)).parent_path().c_str(),nullptr,nullptr,SW_SHOWNORMAL);
    }
    ImGui::SameLine();
    if (ImGui::Button("Extract Materials..."))
    {
        const auto directory = FileDialogs::OpenFolder();
        if (!directory.empty()) ModelAsset::ExtractMaterials(reference,directory,m_Message);
    }
    if (!m_Message.empty()) ImGui::TextWrapped("%s",m_Message.c_str());
}
