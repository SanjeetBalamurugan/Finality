#include "AssetInspector.h"
#include <Editor/ExternalTools.h>
#include "EntityInspector.h"
#include <ImGUI/AssetPreview.h>
#include <fstream>
#include <Renderer/ShaderReflection.h>

void FINALITY::AssetInspector::Clear()
{
    m_Selected = 0;
    m_Modified = 0;
    m_Material = MaterialComponent();
    m_Source.clear();
    m_EntityCount = 0;
}

void FINALITY::AssetInspector::Draw(uint64_t selected, AssetPicker& picker, bool* open)
{
    ImGui::Begin("Inspector", open);
    const auto* record = AssetRegistry::Find(selected);
    if (!record)
    {
        Clear();
        ImGui::TextWrapped("This asset is no longer in the active project.");
        ImGui::End();
        return;
    }
    const auto asset = *record;
    const std::string reference = "asset://" + std::to_string(asset.ID);
    const auto path = AssetRegistry::Resolve(reference);
    if (m_Selected != selected || m_Modified != asset.Modified)
    {
        Clear();
        m_Selected = selected;
        m_Modified = asset.Modified;
        if (asset.Status == "Ready")
        {
            if (asset.Type == ProjectAssetType::Material)
            {
                if (auto material = AssetManager::CreateAsset<MaterialAsset>(reference, Renderer::GetDevice()))
                {
                    m_Material.MaterialIdentifier = reference;
                    m_Material.MaterialInstance = material->CreateInstance();
                }
            }
            if (asset.Type == ProjectAssetType::Shader || asset.Type == ProjectAssetType::Script)
            {
                std::ifstream stream(path, std::ios::binary);
                m_Source.resize(128 * 1024);
                stream.read(m_Source.data(), static_cast<std::streamsize>(m_Source.size()));
                m_Source.resize(static_cast<size_t>(stream.gcount()));
            }
            if (asset.Type == ProjectAssetType::Scene || asset.Type == ProjectAssetType::Prefab)
                try { m_EntityCount = YAML::LoadFile(path)["Entities"].size(); }
                catch (const std::exception&) {}
        }
    }
    const char* types[] = {"File", "Texture", "Model", "Material", "Shader", "Scene", "Native Script Source", "Prefab"};
    ImGui::TextUnformatted(std::filesystem::path(asset.Path).filename().string().c_str());
    ImGui::TextDisabled("%s", types[static_cast<int>(asset.Type)]);
    ImGui::Separator();
    if (asset.Status == "Ready")
        if (void* handle = AssetPreview::RenderedHandle(reference,asset.Type))
        {
            const float width = std::min(ImGui::GetContentRegionAvail().x,240.0f);
            ImGui::Image(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(handle)),{width,width});
        }
    ImGui::TextWrapped("Project path: %s", asset.Path.c_str());
    ImGui::TextWrapped("Asset ID: %llu", static_cast<unsigned long long>(asset.ID));
    ImGui::TextWrapped("Status: %s", asset.Status.c_str());
    if (!asset.Source.empty()) ImGui::TextWrapped("Original source: %s", asset.Source.c_str());
    if (!asset.Diagnostic.empty()) ImGui::TextWrapped("%s", asset.Diagnostic.c_str());
    if (asset.Status == "Missing" || !std::filesystem::is_regular_file(path))
    {
        ImGui::TextWrapped("The asset file is missing. Refresh the project asset registry after restoring it.");
        ImGui::End();
        return;
    }
    ImGui::Separator();
    if (asset.Type == ProjectAssetType::Material)
        DrawMaterialProperties(m_Material, picker, [this,selected]() -> MaterialComponent* { return m_Selected == selected && AssetRegistry::Find(selected) ? &m_Material : nullptr; });
    else if (asset.Type == ProjectAssetType::Texture)
    {
        if (auto texture = AssetManager::CreateAsset<Texture>(reference))
        {
            ImGui::Text("Dimensions: %u x %u", texture->GetWidth(), texture->GetHeight());
            ImGui::Text("Mip levels: %u", texture->GetMipLevels());
            if (void* handle = AssetPreview::TextureHandle(reference))
            {
                const float width = std::min(ImGui::GetContentRegionAvail().x, 320.0f);
                const float aspect = static_cast<float>(texture->GetHeight()) / texture->GetWidth();
                ImGui::Image(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(handle)), {width,width * aspect});
            }
        }
    }
    else if (asset.Type == ProjectAssetType::Model && asset.Status == "Ready")
    {
        try
        {
            const auto model = ModelAsset::Load(reference);
            ImGui::Text("%zu nodes, %zu submeshes, %zu material slots",model->Nodes.size(),model->Meshes.size(),model->Materials.size());
            const auto bounds = model->Maximum - model->Minimum;
            ImGui::Text("Size: %.3f, %.3f, %.3f",bounds.x,bounds.y,bounds.z);
            if (ImGui::CollapsingHeader("Submeshes"))
                for (const auto& mesh : model->Meshes)
                    ImGui::BulletText("%s: %zu triangles, %s",mesh.Name.c_str(),mesh.Geometry.Indices.size()/3,
                        mesh.Material < model->Materials.size() ? model->Materials[mesh.Material].Name.c_str() : "Default");
            if (ImGui::CollapsingHeader("Model hierarchy"))
            {
                std::vector<int> depths;
                for (const auto& node : model->Nodes)
                {
                    const int depth = node.Parent < 0 ? 0 : depths.at(node.Parent) + 1;
                    depths.push_back(depth);
                    ImGui::BulletText("%*s%s",2 * std::min(depth,20),"",node.Name.c_str());
                }
            }
            for (const auto& diagnostic : model->Diagnostics) ImGui::TextWrapped("%s",diagnostic.c_str());
        }
        catch (const std::exception& error) { ImGui::TextWrapped("%s",error.what()); }
    }
    else if (asset.Type == ProjectAssetType::Scene || asset.Type == ProjectAssetType::Prefab) ImGui::Text("Entities: %zu", m_EntityCount);
    else if (asset.Type == ProjectAssetType::Shader || asset.Type == ProjectAssetType::Script)
    {
        if (asset.Type == ProjectAssetType::Shader && ImGui::CollapsingHeader("Shader Uniforms", ImGuiTreeNodeFlags_DefaultOpen))
        {
            try
            {
                const auto uniforms = ShaderReflection::Reflect(path);
                if (uniforms.empty()) ImGui::TextDisabled("This shader has no uniforms.");
                for (const auto& uniform : uniforms)
                {
                    ImGui::PushID((uniform.Block + "." + uniform.Name).c_str());
                    ImGui::TextUnformatted(uniform.Name.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("(%s)", uniform.Type.c_str());
                    if (uniform.Storage == "Texture")
                        ImGui::TextDisabled("  Set %u, binding %u", uniform.Set, uniform.Binding);
                    else
                        ImGui::TextDisabled("  %s, offset %u, %u bytes", uniform.Storage.c_str(), uniform.Offset, uniform.Size);
                    if (uniform.Editable) ImGui::TextDisabled("  Editable on the material");
                    ImGui::PopID();
                }
            }
            catch (const std::exception& error) { ImGui::TextWrapped("%s", error.what()); }
        }
        if (asset.Type == ProjectAssetType::Shader && ShaderCompiler::IsSourceShader(path) && ImGui::Button("Compile Shader")) ShaderCompiler::EnsureCompiled(path, true);
        if (asset.Type == ProjectAssetType::Script)
        {
            if (ImGui::Button("Open Script")) ExternalTools::OpenScript(path);
            if (ImGui::BeginPopupContextWindow("Script Actions", ImGuiPopupFlags_MouseButtonRight))
            {
                if (ImGui::MenuItem("Open Script")) ExternalTools::OpenScript(path);
                ImGui::EndPopup();
            }
            ImGui::TextWrapped("Native source must be compiled and registered before its component can be attached. Registered fields appear on the attached component.");
            if (ImGui::CollapsingHeader("Available compiled script types"))
                for (const auto& [name, definition] : ScriptRegistry::GetDefinitions())
                {
                    ImGui::TextUnformatted(name.c_str());
                    for (const auto& property : definition.Properties) ImGui::BulletText("%s", property.Name.c_str());
                }
        }
        if (ImGui::CollapsingHeader("Source", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginChild("Source Text", {0,240}, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextUnformatted(m_Source.data(), m_Source.data() + m_Source.size());
            ImGui::EndChild();
        }
    }
    ImGui::End();
}
