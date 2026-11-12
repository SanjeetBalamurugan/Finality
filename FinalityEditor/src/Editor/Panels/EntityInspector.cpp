#include "EntityInspector.h"
#include <Editor/ExternalTools.h>
#include <ImGUI/AssetPreview.h>
#include <algorithm>
#include <array>
#include <optional>

namespace
{
    using namespace FINALITY;


    bool BeginProperties(const char* id)
    {
        if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings))
            return false;
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 0.39f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.61f);
        return true;
    }

    void PropertyRow(const char* label)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        const float width = ImGui::GetContentRegionAvail().x;
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        if (ImGui::CalcTextSize(label).x > width && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", label);
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0f);
    }

    void ReadOnlyValue(std::string value)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::InputText("##ReadOnly", value.data(), value.size() + 1, ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor();
    }

    bool VectorField(const char* id, glm::vec3& value, float speed)
    {
        const float width = ImGui::GetContentRegionAvail().x;
        if (width < ImGui::GetFontSize() * 6.0f)
            return ImGui::DragFloat3(id, &value.x, speed, 0, 0, "%.6g");

        const char* axes[] = {"X", "Y", "Z"};
        constexpr float gap = 3.0f;
        const float labelsWidth = ImGui::CalcTextSize("XYZ").x;
        const float fieldWidth = std::max(1.0f, (width - labelsWidth - 5 * gap) / 3);
        bool changed = false;
        ImGui::PushID(id);
        for (int axis = 0; axis < 3; ++axis)
        {
            if (axis > 0)
                ImGui::SameLine(0, gap);
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%s", axes[axis]);
            ImGui::SameLine(0, gap);
            ImGui::PushID(axis);
            ImGui::SetNextItemWidth(fieldWidth);
            changed |= ImGui::DragFloat("##Axis", &value[axis], speed, 0, 0, "%.6g");
            ImGui::PopID();
        }
        ImGui::PopID();
        return changed;
    }

    const char* FieldAssetType(ProjectAssetType type)
    {
        switch (type)
        {
        case ProjectAssetType::Texture: return "Texture";
        case ProjectAssetType::Model: return "Mesh";
        case ProjectAssetType::Material: return "Material";
        case ProjectAssetType::Shader: return "Shader";
        case ProjectAssetType::Scene: return "Scene";
        case ProjectAssetType::Script: return "Script";
        default: return "Asset";
        }
    }

    void AssetField(const char* label, ProjectAssetType type, const std::string& reference,
        AssetPicker& picker, std::function<void(const std::string&)> assign, const std::string& extension = {})
    {
        ImGui::PushID(label);
        PropertyRow(label);
        std::string name = std::filesystem::path(AssetRegistry::Resolve(reference)).filename().string();
        if (reference.empty()) name = std::string("None (") + FieldAssetType(type) + ")";
        else if (name.empty()) name = "Missing asset";
        if (reference.starts_with("builtin://")) name = reference.substr(10);
        const float height = ImGui::GetFrameHeight();
        const float valueWidth = std::max(1.0f, ImGui::GetContentRegionAvail().x - height - 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, {0.0f, 0.5f});
        bool openPicker = ImGui::Button((name + "###AssetValue").c_str(), {valueWidth, height});
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", name.c_str());
        if (ImGui::BeginDragDropTarget())
        {
            if (const auto* payload = ImGui::AcceptDragDropPayload("FinalityAsset"))
                if (payload->DataSize == sizeof(uint64_t))
                    if (const auto* record = AssetRegistry::Find(*static_cast<const uint64_t*>(payload->Data)); record && record->Type == type && (extension.empty() || std::filesystem::path(record->Path).extension() == extension))
                        assign("asset://" + std::to_string(record->ID));
            ImGui::EndDragDropTarget();
        }
        ImGui::SameLine(0, 1.0f);
        openPicker |= ImGui::Button("##PickAsset", {height, height});
        const auto min = ImGui::GetItemRectMin();
        const ImVec2 center{min.x + height * 0.5f, min.y + height * 0.5f};
        auto* draw = ImGui::GetWindowDrawList();
        const auto color = ImGui::GetColorU32(ImGuiCol_Text);
        draw->AddCircle(center, height * 0.24f, color);
        draw->AddCircleFilled(center, height * 0.08f, color);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Select %s", FieldAssetType(type));
        if (openPicker)
            picker.Open(type, reference, std::move(assign), extension);
        ImGui::PopID();
    }

    void DrawComponentIcon(ImVec2 origin, float size, const char* icon)
    {
        auto* draw = ImGui::GetWindowDrawList();
        const float inset = size * 0.14f;
        const auto textSize = ImGui::CalcTextSize(icon);
        const auto background = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
        const auto foreground = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        draw->AddRectFilled({origin.x + inset, origin.y + inset},
            {origin.x + size - inset, origin.y + size - inset}, ImGui::GetColorU32(background), 2.0f);
        draw->AddText({origin.x + (size - textSize.x) * 0.5f, origin.y + (size - textSize.y) * 0.5f},
            ImGui::GetColorU32(foreground), icon);
    }

    bool DrawComponentBar(const std::string& title, const char* icon, bool* enabled = nullptr, void* preview = nullptr)
    {
        ImGui::Spacing();
        auto* storage = ImGui::GetStateStorage();
        const auto state = ImGui::GetID("##Expanded");
        bool open = storage->GetBool(state, true);
        const float height = ImGui::GetFrameHeight();
        const auto start = ImGui::GetCursorScreenPos();
        auto* draw = ImGui::GetWindowDrawList();
        const ImVec2 end{start.x + ImGui::GetContentRegionAvail().x, start.y + height};
        draw->AddRectFilled(start, end, ImGui::GetColorU32(ImGuiCol_Header));
        draw->AddLine(start, {end.x, start.y}, ImGui::GetColorU32(ImGuiCol_Separator));
        draw->AddLine({start.x, end.y}, end, ImGui::GetColorU32(ImGuiCol_Separator));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        if (ImGui::ArrowButton("##Disclosure", open ? ImGuiDir_Down : ImGuiDir_Right))
            open = !open;
        ImGui::SameLine(0, 2.0f);
        if (preview)
        {
            ImGui::Image(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(preview)), {height, height});
        }
        else
        {
            DrawComponentIcon(ImGui::GetCursorScreenPos(), height, icon);
            ImGui::Dummy({height, height});
        }
        if (enabled)
        {
            ImGui::SameLine(0, 4.0f);
            ImGui::Checkbox("##Enabled", enabled);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Enable script");
        }
        ImGui::SameLine(0, 2.0f);
        const float titleWidth = std::max(1.0f, ImGui::GetContentRegionAvail().x - height);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, {0.0f, 0.5f});
        if (ImGui::Button((title + "###ComponentTitle").c_str(), {titleWidth, height}))
            open = !open;
        ImGui::PopStyleVar();
        if (ImGui::IsItemHovered() && ImGui::CalcTextSize(title.c_str()).x > titleWidth)
            ImGui::SetTooltip("%s", title.c_str());
        ImGui::OpenPopupOnItemClick("Component Actions");
        ImGui::SameLine(0, 0.0f);
        if (ImGui::Button("##Actions", {height, height}))
            ImGui::OpenPopup("Component Actions");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Component actions");
        const auto menu = ImGui::GetItemRectMin();
        for (float offset : {-0.19f, 0.0f, 0.19f})
            draw->AddCircleFilled({menu.x + height * 0.5f, menu.y + height * (0.5f + offset)},
                height * 0.055f, ImGui::GetColorU32(ImGuiCol_Text));
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        storage->SetBool(state, open);
        return open;
    }

    template<typename T>
    std::optional<T>& Clipboard()
    {
        static std::optional<T> copied;
        return copied;
    }

    template<typename T>
    bool ComponentHeader(Scene& scene, entt::entity entity, const std::string& name, const char* icon, void* preview = nullptr)
    {
        auto& registry = scene.GetRegistry();
        const bool open = DrawComponentBar(name, icon, nullptr, preview);
        bool removed = false;
        if (ImGui::BeginPopup("Component Actions"))
        {
            if (ImGui::MenuItem("Reset"))
            {
                Renderer::GetDevice()->WaitIdle();
                if constexpr (std::is_same_v<T, CameraComponent>) Renderer::SetActiveCamera(nullptr);
                registry.replace<T>(entity);
            }
            auto& copied = Clipboard<T>();
            if (ImGui::MenuItem("Copy Values")) copied = registry.get<T>(entity);
            if (ImGui::MenuItem("Paste Values", nullptr, false, copied.has_value()))
            {
                Renderer::GetDevice()->WaitIdle();
                if constexpr (std::is_same_v<T, CameraComponent>) Renderer::SetActiveCamera(nullptr);
                registry.replace<T>(entity, *copied);
            }
            if constexpr (!std::is_same_v<T, TransformComponent>)
            {
                if (ImGui::MenuItem("Remove"))
                {
                    Renderer::GetDevice()->WaitIdle();
                    if constexpr (std::is_same_v<T, CameraComponent>) Renderer::SetActiveCamera(nullptr);
                    registry.remove<T>(entity);
                    removed = true;
                }
            }
            ImGui::EndPopup();
        }
        return open && !removed;
    }

    void MaterialProperties(MaterialComponent& component, AssetPicker& picker, std::function<MaterialComponent*()> target)
    {
        auto asset = AssetManager::CreateAsset<MaterialAsset>(component.MaterialIdentifier, Renderer::GetDevice());
        if (!asset || !component.MaterialInstance) { ImGui::TextUnformatted("Missing material. See Console."); return; }
        auto& definition = asset->GetDefinition();
        auto shader = [&](const char* label, const std::string& reference, const char* extension, bool vertex)
        {
            AssetField(label, ProjectAssetType::Shader, reference, picker, [target,vertex](const std::string& selected)
            {
                if (auto* current = target())
                    if (auto material = AssetManager::CreateAsset<MaterialAsset>(current->MaterialIdentifier, Renderer::GetDevice()))
                        (vertex ? material->GetDefinition().VertexShader : material->GetDefinition().FragmentShader) = selected;
            }, extension);
        };
        if (BeginProperties("Shaders"))
        {
            PropertyRow("Shader Type");
            ReadOnlyValue(definition.Type);
            shader("Vertex Shader", definition.VertexShader, ".vert", true);
            shader("Fragment Shader", definition.FragmentShader, ".frag", false);
            ImGui::EndTable();
        }
        if (ImGui::CollapsingHeader("Pipeline") && BeginProperties("Pipeline Properties"))
        {
            PropertyRow("Blending");
            ImGui::Checkbox("##Blending", &definition.EnableBlending);
            const char* choices[] = {"None", "Front", "Back"};
            int culling = definition.Culling == CullMode::Front ? 1 : definition.Culling == CullMode::Back ? 2 : 0;
            PropertyRow("Culling");
            if (ImGui::Combo("##Culling", &culling, choices, 3))
                definition.Culling = culling == 1 ? CullMode::Front : culling == 2 ? CullMode::Back : CullMode::None;
            ImGui::EndTable();
        }
        if (!definition.Properties.empty() && ImGui::CollapsingHeader("Shader Uniforms", ImGuiTreeNodeFlags_DefaultOpen)
            && BeginProperties("Parameter Values"))
        {
            for (const auto& property : definition.Properties)
            {
                ImGui::PushID(property.Name.c_str());
                PropertyRow(property.Name.c_str());
                if (property.Type == MaterialParameterType::Float)
                {
                    float value = 0;
                    if (component.MaterialInstance->TryGetUniform1f(property.Name, value) && ImGui::DragFloat("##Value", &value, 0.01f, 0, 0, "%.6g"))
                        component.MaterialInstance->SetUniform1f(property.Name, value);
                }
                else if (property.Type == MaterialParameterType::Vec4)
                {
                    glm::vec4 value;
                    if (component.MaterialInstance->TryGetUniformVec4(property.Name, value))
                    {
                        const bool color = MatchesSearch(property.Name, "color") || MatchesSearch(property.Name, "tint");
                        const bool changed = color ? ImGui::ColorEdit4("##Value", &value.x, ImGuiColorEditFlags_Float) :
                            ImGui::DragFloat4("##Value", &value.x, 0.01f, 0, 0, "%.6g");
                        if (changed) component.MaterialInstance->SetUniformVec4(property.Name, value);
                    }
                }
                else
                {
                    glm::mat4 value;
                    if (component.MaterialInstance->TryGetUniformMat4(property.Name, value)
                        && ImGui::TreeNodeEx("Edit Matrix", ImGuiTreeNodeFlags_NoTreePushOnOpen))
                    {
                        bool changed = false;
                        for (int column = 0; column < 4; ++column)
                        {
                            const auto label = "Column " + std::to_string(column);
                            PropertyRow(label.c_str());
                            ImGui::PushID(column);
                            changed |= ImGui::DragFloat4("##Column", &value[column].x, 0.01f, 0, 0, "%.6g");
                            ImGui::PopID();
                        }
                        if (changed) component.MaterialInstance->SetUniformMat4(property.Name, value);
                    }
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        if (!definition.TextureBinding.empty() && BeginProperties("Texture Properties"))
        {
            AssetField(definition.TextureBinding.c_str(), ProjectAssetType::Texture, definition.TexturePath, picker, [target](const std::string& selected)
            {
                auto* current = target();
                if (!current || !current->MaterialInstance) return;
                if (auto material = AssetManager::CreateAsset<MaterialAsset>(current->MaterialIdentifier, Renderer::GetDevice()))
                {
                    auto texture = AssetManager::CreateAsset<Texture>(selected.empty() ? "builtin://white" : selected);
                    if (!selected.empty() && !texture) return;
                    material->GetDefinition().TexturePath = selected;
                    current->MaterialInstance->SetTexture(material->GetDefinition().TextureBinding, std::move(texture));
                }
            });
            ImGui::EndTable();
        }
        if (ImGui::Button("Apply and Save Material"))
        {
            asset->UpdateDefaultsFromInstance(*component.MaterialInstance);
            const auto path = AssetRegistry::Resolve(component.MaterialIdentifier);
            if (MaterialSerializer::Serialize(definition, path))
            {
                Renderer::GetDevice()->WaitIdle();
                AssetManager::InvalidateAssetPath(path);
                auto updated = AssetManager::CreateAsset<MaterialAsset>(component.MaterialIdentifier, Renderer::GetDevice());
                if (updated) component.MaterialInstance = updated->CreateInstance();
                AssetRegistry::Scan();
            }
        }
    }

    void DrawScriptProperties(Scene* scene, entt::entity entity, ScriptStorage::ScriptInstance& script, AssetPicker& picker)
    {
        const auto* definition = ScriptRegistry::Find(script.TypeName);
        if (BeginProperties("Script Properties"))
        {
            PropertyRow("Script");
            ReadOnlyValue(script.TypeName);
            if (definition && !definition->SourceFile.empty() && ImGui::BeginPopupContextItem("Script Source Actions"))
            {
                if (ImGui::MenuItem("Open Script")) ExternalTools::OpenScript(definition->SourceFile);
                ImGui::EndPopup();
            }
            if (definition && script.Instance)
            {
                std::map<std::string, bool> groups;
                for (const auto& property : definition->Properties)
                {
                    bool visible = true;
                    size_t fieldStart = 0;
                    for (auto separator = property.Name.find('.', fieldStart); separator != std::string::npos;
                        separator = property.Name.find('.', fieldStart))
                    {
                        const auto group = property.Name.substr(0, separator);
                        auto found = groups.find(group);
                        if (found == groups.end())
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::PushID(group.c_str());
                            const bool open = ImGui::TreeNodeEx("##Struct", ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                ImGuiTreeNodeFlags_DefaultOpen, "%s", property.Name.substr(fieldStart, separator - fieldStart).c_str());
                            ImGui::PopID();
                            found = groups.emplace(group, open).first;
                        }
                        if (!found->second) { visible = false; break; }
                        fieldStart = separator + 1;
                    }
                    if (!visible) continue;
                    const auto label = property.Name.substr(fieldStart);
                    auto value = property.Get(*script.Instance);
                    bool changed = false;
                    ImGui::PushID(property.Name.c_str());
                    std::visit([&](auto& item)
                    {
                        using T = std::decay_t<decltype(item)>;
                        if constexpr (std::is_same_v<T, ScriptAssetReference>)
                        {
                            AssetField(label.c_str(), item.Type, item.Reference, picker,
                                [scene, entity, instance = script.Instance, name = property.Name, type = item.Type](const std::string& reference)
                                {
                                    if (SceneManager::GetInstance().GetActiveScene() != scene || !scene->GetRegistry().valid(entity))
                                        return;
                                    const auto* storage = scene->GetRegistry().try_get<ScriptStorage>(entity);
                                    if (!storage) return;
                                    const auto entry = std::find_if(storage->Scripts.begin(), storage->Scripts.end(),
                                        [&](const auto& candidate) { return candidate.Instance == instance; });
                                    if (entry == storage->Scripts.end()) return;
                                    const auto* current = ScriptRegistry::Find(entry->TypeName);
                                    if (!current) return;
                                    const auto field = std::find_if(current->Properties.begin(), current->Properties.end(),
                                        [&](const auto& candidate) { return candidate.Name == name; });
                                    if (field != current->Properties.end())
                                        field->Set(*instance, ScriptAssetReference{type, reference});
                                });
                        }
                        else
                        {
                            PropertyRow(label.c_str());
                            if constexpr (std::is_same_v<T, bool>) changed = ImGui::Checkbox("##Value", &item);
                            else if constexpr (std::is_same_v<T, int>) changed = ImGui::InputInt("##Value", &item, 0, 0);
                            else if constexpr (std::is_same_v<T, float>) changed = ImGui::InputFloat("##Value", &item, 0, 0, "%.6g");
                            else if constexpr (std::is_same_v<T, glm::vec3>)
                            {
                                changed = VectorField("##Value", item, 0.1f);
                            }
                            else changed = EditText("##Value", item);
                        }
                    }, value);
                    ImGui::PopID();
                    if (changed) property.Set(*script.Instance, value);
                }
            }
            ImGui::EndTable();
        }
        if (!definition || !script.Instance)
            ImGui::TextWrapped("Script unavailable. Compile and register this type to edit its saved properties.");
    }

    void DrawAddComponent(Scene& scene, entt::entity entity)
    {
        ImGui::Separator();
        if (ImGui::Button("Add Component", {-1, 0}))
            ImGui::OpenPopup("Add Component");
        if (!ImGui::IsPopupOpen("Add Component")) return;

        const auto screen = ImGui::GetMainViewport()->WorkSize;
        ImGui::SetNextWindowSize({std::min(285.0f, screen.x), std::min(300.0f, screen.y * 0.7f)});
        if (!ImGui::BeginPopup("Add Component", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
            return;

        static std::array<char, 160> query{};
        if (ImGui::IsWindowAppearing())
        {
            query.fill(0);
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(-1.0f);
        const bool accept = ImGui::InputTextWithHint("##Search", "Search components", query.data(), query.size(),
            ImGuiInputTextFlags_EnterReturnsTrue);
        const std::string search(query.data());
        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, {0.5f, 0.5f});
        ImGui::SeparatorText(search.empty() ? "Components" : "Search");
        ImGui::PopStyleVar();

        if (ImGui::BeginChild("Results", {0, 0}, ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground))
        {
            auto& registry = scene.GetRegistry();
            bool hasResults = false;
            bool added = false;
            ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
            auto entry = [&](const char* name, const char* icon, bool present, auto add)
            {
                if (added || present || !MatchesSearch(name, search)) return;
                const bool firstMatch = !hasResults && !search.empty();
                hasResults = true;
                ImGui::PushID(icon);
                ImGui::PushID(name);
                const auto origin = ImGui::GetCursorScreenPos();
                const auto label = std::string("     ") + name;
                const bool clicked = ImGui::Selectable(label.c_str(), firstMatch, ImGuiSelectableFlags_NoAutoClosePopups);
                DrawComponentIcon(origin, ImGui::GetTextLineHeight(), icon);
                ImGui::PopID();
                ImGui::PopID();
                if (clicked || (accept && firstMatch))
                {
                    add();
                    added = true;
                }
            };
            entry("Mesh", "M", registry.all_of<MeshComponent>(entity), [&] { registry.emplace<MeshComponent>(entity); });
            entry("Material", "M", registry.all_of<MaterialComponent>(entity), [&] { registry.emplace<MaterialComponent>(entity); });
            entry("Camera", "C", registry.all_of<CameraComponent>(entity), [&] { registry.emplace<CameraComponent>(entity); });
            const auto* storage = registry.try_get<ScriptStorage>(entity);
            for (const auto& [name, definition] : ScriptRegistry::GetDefinitions())
            {
                const bool present = storage && std::any_of(storage->Scripts.begin(), storage->Scripts.end(),
                    [&](const auto& script) { return script.TypeName == name; });
                entry(name.c_str(), "#", present, [&] { ScriptRegistry::Attach(scene, entity, name); });
            }
            ImGui::PopStyleColor();
            if (!hasResults)
                ImGui::TextDisabled("No matching components");
            if (added)
                ImGui::CloseCurrentPopup();
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
}

void FINALITY::DrawMaterialProperties(MaterialComponent& component, AssetPicker& picker, std::function<MaterialComponent*()> target)
{
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {5.0f, 1.0f});
    MaterialProperties(component, picker, std::move(target));
    ImGui::PopStyleVar();
}

void FINALITY::ClearInspectorClipboard()
{
    Clipboard<MeshComponent>().reset();
    Clipboard<MaterialComponent>().reset();
    Clipboard<CameraComponent>().reset();
    Clipboard<TransformComponent>().reset();
}

void FINALITY::DrawEntityInspector(Scene* scene, entt::entity selected, AssetPicker& picker, bool* open)
{
    if (!ImGui::Begin("Inspector", open))
    {
        ImGui::End();
        return;
    }
    if (!scene || !scene->GetRegistry().valid(selected))
    {
        ImGui::TextWrapped("Select an entity in the Hierarchy to edit its components.");
        ImGui::End();
        return;
    }
    auto& registry = scene->GetRegistry();
    auto valid = [scene,selected]() { return SceneManager::GetInstance().GetActiveScene() == scene && scene->GetRegistry().valid(selected); };
    ImGui::PushID(static_cast<int>(entt::to_integral(selected)));
    if (auto* tag = registry.try_get<TagComponent>(selected))
    {
        ImGui::SetNextItemWidth(-1.0f);
        EditText("##EntityName", tag->tag);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Entity name");
    }
    ImGui::PushID("Transform");
    if (ComponentHeader<TransformComponent>(*scene, selected, "Transform", "T") && BeginProperties("Transform Properties"))
    {
        auto& transform = registry.get<TransformComponent>(selected);
        PropertyRow("Position");
        VectorField("##Position", transform.Position, 0.05f);
        PropertyRow("Rotation");
        VectorField("##Rotation", transform.Rotation, 0.5f);
        PropertyRow("Scale");
        VectorField("##Scale", transform.Scale, 0.01f);
        transform.Matrix = transform.GetTransformMatrix();
        ImGui::EndTable();
    }
    ImGui::PopID();
    ImGui::PushID("Mesh");
    if (registry.all_of<MeshComponent>(selected) && ComponentHeader<MeshComponent>(*scene, selected, "Mesh", "M")
        && BeginProperties("Mesh Properties"))
    {
        auto& mesh = registry.get<MeshComponent>(selected);
        AssetField("Mesh", ProjectAssetType::Model, mesh.MeshIdentifier, picker, [valid,scene,selected](const std::string& reference)
        {
            if (!valid()) return;
            auto* mesh = scene->GetRegistry().try_get<MeshComponent>(selected);
            if (!mesh) return;
            auto data = reference.empty() ? nullptr : Mesh::Create(reference);
            if (!reference.empty() && !data) return;
            Renderer::GetDevice()->WaitIdle();
            mesh->MeshIdentifier = reference;
            mesh->MeshData = std::move(data);
        });
        ImGui::EndTable();
    }
    ImGui::PopID();
    ImGui::PushID("Camera");
    if (registry.all_of<CameraComponent>(selected) && ComponentHeader<CameraComponent>(*scene, selected, "Camera", "C")
        && BeginProperties("Camera Properties"))
    {
        auto& camera = registry.get<CameraComponent>(selected);
        PropertyRow("Primary");
        if (ImGui::Checkbox("##Primary", &camera.IsPrimary) && camera.IsPrimary)
            for (auto entity : registry.view<CameraComponent>()) if (entity != selected) registry.get<CameraComponent>(entity).IsPrimary = false;
        PropertyRow("Field of View");
        bool changed = ImGui::SliderFloat("##FieldOfView", &camera.FOV, 10, 120, "%.6g");
        PropertyRow("Near Clip");
        changed |= ImGui::DragFloat("##NearClip", &camera.NearClip, 0.01f, 0.01f, camera.FarClip - 0.01f, "%.6g");
        PropertyRow("Far Clip");
        changed |= ImGui::DragFloat("##FarClip", &camera.FarClip, 1, camera.NearClip + 0.01f, 100000, "%.6g");
        if (changed) { Renderer::SetActiveCamera(nullptr); camera.RebuildCamera(); }
        ImGui::EndTable();
    }
    ImGui::PopID();
    ImGui::PushID("Scripts");
    if (auto* storage = registry.try_get<ScriptStorage>(selected))
        for (size_t index = 0; index < storage->Scripts.size(); ++index)
        {
            auto& script = storage->Scripts[index];
            if (script.TypeName.empty()) continue;
            ImGui::PushID(script.TypeName.c_str());
            const bool open = DrawComponentBar(script.TypeName + " (Script)", "#", &script.Enabled);
            bool remove = false;
            if (ImGui::BeginPopup("Component Actions"))
            {
                const auto* definition = ScriptRegistry::Find(script.TypeName);
                if (ImGui::MenuItem("Open Script", nullptr, false, definition && !definition->SourceFile.empty()))
                    ExternalTools::OpenScript(definition->SourceFile);
                remove = ImGui::MenuItem("Remove");
                ImGui::EndPopup();
            }
            if (open && !remove)
                DrawScriptProperties(scene, selected, script, picker);
            ImGui::PopID();
            if (remove) { ScriptRegistry::Remove(*scene, selected, index); break; }
        }
    ImGui::PopID();
    ImGui::PushID("Material");
    if (const auto* material = registry.try_get<MaterialComponent>(selected))
    {
        const auto asset = AssetManager::CreateAsset<MaterialAsset>(material->MaterialIdentifier, Renderer::GetDevice());
        const auto title = asset ? asset->GetDefinition().Name + " (Material)" : "Material";
        void* preview = nullptr;
        if (asset && ImGui::IsRectVisible({ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight()}))
            preview = AssetPreview::RenderedHandle(material->MaterialIdentifier, ProjectAssetType::Material);
        if (ComponentHeader<MaterialComponent>(*scene, selected, title, "M", preview))
        {
            auto target = [valid,scene,selected]() -> MaterialComponent*
            {
                return valid() ? scene->GetRegistry().try_get<MaterialComponent>(selected) : nullptr;
            };
            auto& current = registry.get<MaterialComponent>(selected);
            if (BeginProperties("Material Reference"))
            {
                AssetField("Material", ProjectAssetType::Material, current.MaterialIdentifier, picker, [target](const std::string& reference)
                {
                    auto* component = target();
                    if (!component) return;
                    auto selectedAsset = reference.empty() ? nullptr : AssetManager::CreateAsset<MaterialAsset>(reference, Renderer::GetDevice());
                    if (!reference.empty() && !selectedAsset) return;
                    Renderer::GetDevice()->WaitIdle();
                    component->MaterialIdentifier = reference;
                    component->MaterialInstance = selectedAsset ? selectedAsset->CreateInstance() : nullptr;
                });
                ImGui::EndTable();
            }
            if (!current.MaterialIdentifier.empty())
                MaterialProperties(current, picker, target);
        }
    }
    ImGui::PopID();
    DrawAddComponent(*scene, selected);
    ImGui::PopID();
    ImGui::End();
}
