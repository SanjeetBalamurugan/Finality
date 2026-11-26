#include "EditorLayer.h"
#include <ImGUI/ImGuizmoBridge.h>
#include <unordered_set>
#include "Prefab.h"

void FINALITY::EditorLayer::DrawHierarchy()
{
    if (!ImGui::Begin("Hierarchy", &m_ShowHierarchy))
    {
        ImGui::End();
        return;
    }
    auto* scene = SceneManager::GetInstance().GetActiveScene();
    if (!scene)
    {
        ImGui::End();
        return;
    }
    auto& registry = scene->GetRegistry();
    if (m_FocusSearch == "Hierarchy")
    {
        ImGui::SetKeyboardFocusHere();
        m_FocusSearch.clear();
    }
    EditText("Search", m_HierarchySearch);
    if (ImGui::Button("Create Entity"))
    {
        ImGui::OpenPopup("Create");
    }
    if (ImGui::BeginPopup("Create"))
    {
        DrawCreateMenu();
        ImGui::EndPopup();
    }
    entt::entity erase = entt::null, duplicate = entt::null, moved = entt::null, parent = entt::null;
    std::unordered_set<entt::entity> visited;
    auto draw = [&](auto&& self, entt::entity entity) -> void
    {
        if (!registry.valid(entity) || !visited.insert(entity).second)
        {
            return;
        }
        const auto* tag = registry.try_get<TagComponent>(entity);
        if (!tag)
        {
            return;
        }
        const auto* relationship = registry.try_get<RelationshipComponent>(entity);
        const auto firstChild = relationship ? relationship->FirstChild : entt::null;
        ImGui::PushID(static_cast<int>(entity));
        auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (entity == m_Session.SelectedEntity)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (firstChild == entt::null)
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        if (!m_HierarchySearch.empty())
        {
            ImGui::SetNextItemOpen(true);
        }
        const bool visible = m_HierarchySearch.empty() || MatchesSearch(tag->tag, m_HierarchySearch) || firstChild != entt::null;
        const bool open = visible && ImGui::TreeNodeEx(tag->tag.c_str(), flags);
        if (visible)
        {
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                m_Session.SelectEntity(entity);
            }
            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("FinalityEntity", &entity, sizeof(entity));
                ImGui::TextUnformatted(tag->tag.c_str());
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginDragDropTarget())
            {
                if (const auto* payload = ImGui::AcceptDragDropPayload("FinalityEntity"))
                {
                    moved = *static_cast<const entt::entity*>(payload->Data);
                    parent = entity;
                }
                if (const auto* payload = ImGui::AcceptDragDropPayload("FinalityAsset"); payload && payload->DataSize == sizeof(uint64_t))
                {
                    const auto* asset = AssetRegistry::Find(*static_cast<const uint64_t*>(payload->Data));
                    if (asset && asset->Type == ProjectAssetType::Script)
                    {
                        for (const auto& [name, definition] : ScriptRegistry::GetDefinitions())
                        {
                            if (std::filesystem::path(definition.SourceFile).lexically_normal() ==
                                std::filesystem::path(AssetRegistry::Resolve("asset://" + std::to_string(asset->ID))).lexically_normal())
                            {
                                ScriptRegistry::Attach(*scene, entity, name);
                            }
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }
            if (ImGui::BeginPopupContextItem("Entity"))
            {
                m_Session.SelectedEntity = entity;
                m_Session.SelectedAsset = 0;
                if (ImGui::MenuItem("Duplicate"))
                {
                    duplicate = entity;
                }
                if (ImGui::MenuItem("Save as Prefab..."))
                {
                    const auto path = FileDialogs::SaveFile("Finality Prefab\0*.fprefab\0\0", "New Prefab.fprefab");
                    if (!path.empty())
                    {
                        SavePrefab(*scene, entity, std::filesystem::path(path).replace_extension(".fprefab"));
                    }
                }
                if (ImGui::MenuItem("Unparent"))
                {
                    moved = entity;
                    parent = entt::null;
                }
                if (ImGui::MenuItem("Delete..."))
                {
                    erase = entity;
                }
                if (ImGui::BeginMenu("Create Child"))
                {
                    const auto previous = m_Session.SelectedEntity;
                    DrawCreateMenu();
                    if (m_Session.SelectedEntity != previous)
                    {
                        moved = m_Session.SelectedEntity;
                        parent = entity;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }
        }
        if (open)
        {
            auto child = firstChild;
            std::unordered_set<entt::entity> siblings;
            while (child != entt::null && registry.valid(child) && siblings.insert(child).second)
            {
                self(self, child);
                const auto* link = registry.try_get<RelationshipComponent>(child);
                child = link ? link->NextSibling : entt::null;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    };
    std::vector<entt::entity> roots;
    for (auto entity : registry.view<TagComponent>())
    {
        const auto* relationship = registry.try_get<RelationshipComponent>(entity);
        if (!relationship || !registry.valid(relationship->Parent))
        {
            roots.push_back(entity);
        }
    }
    for (auto entity : roots)
    {
        draw(draw, entity);
    }
    if (ImGui::BeginPopup("Rename Entity"))
    {
        if (registry.valid(m_Session.SelectedEntity))
        {
            EditText("Name", registry.get<TagComponent>(m_Session.SelectedEntity).tag);
        }
        if (ImGui::Button("Done"))
        {
            m_Session.Checkpoint.Request();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupContextWindow("Hierarchy Actions", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        DrawCreateMenu();
        ImGui::EndPopup();
    }
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        m_Session.SelectedEntity = entt::null;
    }
    if (moved != entt::null && registry.valid(moved))
    {
        auto world = scene->GetWorldTransform(moved);
        scene->GetEntity(moved).SetParent(scene->GetEntity(parent));
        if (registry.get<RelationshipComponent>(moved).Parent == parent)
        {
            if (registry.valid(parent))
            {
                world = glm::inverse(scene->GetWorldTransform(parent)) * world;
            }
            auto& transform = registry.get<TransformComponent>(moved);
            ImGuizmoBridge::SetLocalTransform(transform, world);
        }
    }
    if (duplicate != entt::null)
    {
        m_Session.SelectedEntity = scene->DuplicateEntity(duplicate).GetHandle();
    }
    static entt::entity pendingDelete = entt::null;
    if (erase != entt::null)
    {
        pendingDelete = erase;
        ImGui::OpenPopup("Delete Entity?");
    }
    if (ImGui::BeginPopupModal("Delete Entity?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Delete the entity and its children?");
        if (ImGui::Button("Delete"))
        {
            scene->DestroyEntity(pendingDelete);
            m_Session.SelectedEntity = entt::null;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::End();
}
