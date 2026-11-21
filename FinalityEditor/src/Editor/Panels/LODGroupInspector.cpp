#include "LODGroupInspector.h"
#include <Scene/Entity.h>
#include <Scene/Components.h>
#include <Renderer/Renderer.h>
#include <imgui.h>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

namespace
{
    void AssignRenderers(FINALITY::Scene& scene, entt::entity root, FINALITY::LODGroupComponent& group, size_t level)
    {
        auto& registry = scene.GetRegistry();
        std::vector<entt::entity> pending{root};
        std::unordered_set<entt::entity> visited;
        while (!pending.empty())
        {
            const auto entity = pending.back(); pending.pop_back();
            if (!registry.valid(entity) || !visited.insert(entity).second) continue;
            if (registry.all_of<FINALITY::MeshComponent,FINALITY::TagComponent>(entity))
            {
                const uint64_t id = registry.get<FINALITY::TagComponent>(entity).uuid;
                for (auto& entry : group.Levels) std::erase(entry.Renderers,id);
                group.Levels[level].Renderers.push_back(id);
            }
            if (const auto* relationship = registry.try_get<FINALITY::RelationshipComponent>(entity))
            {
                auto child = relationship->FirstChild;
                while (registry.valid(child) && pending.size() < 100000)
                {
                    pending.push_back(child);
                    const auto* next = registry.try_get<FINALITY::RelationshipComponent>(child);
                    child = next ? next->NextSibling : entt::null;
                }
            }
        }
    }
}

void FINALITY::DrawLODGroupProperties(Scene& scene, entt::entity entity)
{
    auto& registry = scene.GetRegistry();
    auto& group = registry.get<LODGroupComponent>(entity);
    ImGui::Checkbox("Enabled",&group.Enabled);
    ImGui::SameLine(); ImGui::Checkbox("Automatic Bounds",&group.AutoBounds);
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##Hysteresis",&group.Hysteresis,0,0.5f,"Transition margin %.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("A margin around each threshold prevents rapid LOD switching.");
    if (!group.AutoBounds)
    {
        ImGui::DragFloat3("Center",&group.LocalCenter.x,0.01f);
        ImGui::DragFloat("Size (m)",&group.LocalSize,0.01f,0.001f,100000);
    }
    if (ImGui::Button("Recalculate Bounds")) RecalculateLODBounds(scene,entity);
    const float width = ImGui::GetContentRegionAvail().x;
    const auto origin = ImGui::GetCursorScreenPos();
    ImGui::Dummy({width,ImGui::GetFrameHeight()});
    auto* draw = ImGui::GetWindowDrawList();
    float previous = 1;
    for (size_t level = 0; level <= group.Levels.size(); ++level)
    {
        const float threshold = level < group.Levels.size() ? group.Levels[level].ScreenRelativeHeight : 0;
        auto color = ImGui::GetStyleColorVec4(level == group.Levels.size() ? ImGuiCol_FrameBg : ImGuiCol_HeaderActive);
        color.w = std::max(0.25f,1.0f-float(level)*0.14f);
        const float left = origin.x+(1-previous)*width, right = origin.x+(1-threshold)*width;
        draw->AddRectFilled({left,origin.y},{right,origin.y+ImGui::GetFrameHeight()},ImGui::GetColorU32(color));
        draw->AddLine({right,origin.y},{right,origin.y+ImGui::GetFrameHeight()},ImGui::GetColorU32(ImGuiCol_Separator));
        previous = threshold;
    }
    ImGui::TextDisabled("Large on screen                   Small / culled");
    ImGui::TextWrapped("Drop renderer entities or a model root from Hierarchy into a level. Each level uses separately authored meshes.");
    std::unordered_map<uint64_t,entt::entity> renderers;
    for (auto renderer : registry.view<TagComponent>()) renderers[registry.get<TagComponent>(renderer).uuid] = renderer;
    int removeLevel = -1;
    for (size_t index = 0; index < group.Levels.size(); ++index)
    {
        ImGui::PushID(static_cast<int>(index));
        auto& level = group.Levels[index];
        ImGui::SeparatorText(("LOD "+std::to_string(index)).c_str());
        float percent = level.ScreenRelativeHeight*100;
        ImGui::SetNextItemWidth(std::max(50.0f,ImGui::GetContentRegionAvail().x-95));
        if (ImGui::DragFloat("##Threshold",&percent,0.1f,0,100,"Above %.1f%% screen height")) level.ScreenRelativeHeight = percent/100;
        ImGui::SameLine(); if (ImGui::SmallButton("Remove LOD")) removeLevel = static_cast<int>(index);
        for (size_t item = 0; item < level.Renderers.size();)
        {
            ImGui::PushID(static_cast<int>(item));
            const auto found = renderers.find(level.Renderers[item]);
            const auto label = found == renderers.end() ? "Missing renderer" : registry.get<TagComponent>(found->second).tag;
            ImGui::SetNextItemWidth(-30);
            ImGui::TextUnformatted(label.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("x")) level.Renderers.erase(level.Renderers.begin()+item);
            else ++item;
            ImGui::PopID();
        }
        ImGui::Button("Drop renderers here",{-1,ImGui::GetFrameHeight()*1.3f});
        if (ImGui::BeginDragDropTarget())
        {
            if (const auto* payload = ImGui::AcceptDragDropPayload("FinalityEntity"))
                if (payload->DataSize == sizeof(entt::entity)) AssignRenderers(scene,*static_cast<const entt::entity*>(payload->Data),group,index);
            ImGui::EndDragDropTarget();
        }
        if (level.Renderers.empty()) ImGui::TextDisabled("No renderers assigned");
        ImGui::PopID();
    }
    if (removeLevel >= 0) group.Levels.erase(group.Levels.begin()+removeLevel);
    if (group.Levels.size() < 8 && ImGui::Button("Add LOD",{-1,0}))
        group.Levels.push_back({group.Levels.empty() ? 0.5f : group.Levels.back().ScreenRelativeHeight*0.5f,{}});
    NormalizeLODGroup(group);
}
