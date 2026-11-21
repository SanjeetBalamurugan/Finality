#include "EditorLayer.h"
#include <ImGUI/ImGuizmoBridge.h>
#include <imgui_internal.h>

namespace
{
    bool Focused(const char* name)
    {
        const auto* window=ImGui::FindWindowByName(name);
        const auto* focused=GImGui->NavWindow;
        return window && focused && (focused==window || focused->RootWindow==window->RootWindow);
    }
}

void FINALITY::EditorLayer::ExecuteEntityCommand(EditorAction action)
{
    auto* scene=SceneManager::GetInstance().GetActiveScene();
    if(!scene || m_Session.State!=EditorState::Edit) return;
    auto& registry=scene->GetRegistry();
    const auto selected=m_Session.SelectedEntity;
    if(action==EditorAction::Paste)
    {
        if(m_EntityClipboard.empty()) return;
        EmptyScene source;
        if(!SceneSerializer(source).DeSerializeFromString(m_EntityClipboard)) return;
        for(auto entity:source.GetRegistry().view<TagComponent>())
        {
            const auto* relationship=source.GetRegistry().try_get<RelationshipComponent>(entity);
            if(relationship && relationship->Parent!=entt::null) continue;
            RecordHistory();
            m_Session.SelectEntity(scene->InstantiateHierarchy(source,entity).GetHandle());
            m_Session.Checkpoint.Request();
            break;
        }
        return;
    }
    if(!registry.valid(selected)) return;
    if(action==EditorAction::Copy || action==EditorAction::Cut)
    {
        EmptyScene snapshot;
        auto root=snapshot.InstantiateHierarchy(*scene,selected);
        ImGuizmoBridge::SetLocalTransform(root.GetComponent<TransformComponent>(),scene->GetWorldTransform(selected));
        m_EntityClipboard=SceneSerializer(snapshot).SerializeToString();
        if(action==EditorAction::Copy) return;
    }
    if(action==EditorAction::Rename)
    {
        m_RenameTarget=selected; m_RenameBuffer=registry.get<TagComponent>(selected).tag;
        m_RenamePending=true; return;
    }
    RecordHistory();
    if(action==EditorAction::Delete || action==EditorAction::Cut) { scene->DestroyEntity(selected); m_Session.SelectEntity(entt::null); }
    else if(action==EditorAction::Duplicate) m_Session.SelectEntity(scene->DuplicateEntity(selected).GetHandle());
    m_Session.Checkpoint.Request();
}

void FINALITY::EditorLayer::HandleShortcuts()
{
    auto& io=ImGui::GetIO();
    const auto& keys=m_Session.Preferences;
    if(io.WantTextInput || io.AppFocusLost || ImGui::IsAnyItemActive() || ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId) || m_CameraCaptured || m_ShortcutCapture>=0) return;
    if(keys.Pressed(EditorAction::Play)) { if(m_Session.State==EditorState::Edit) OnPlay(); else OnStop(); return; }
    if(keys.Pressed(EditorAction::Pause)) { if(m_Session.State==EditorState::Play) OnPause(); else if(m_Session.State==EditorState::Paused) OnResume(); return; }
    if(keys.Pressed(EditorAction::Stop)) { if(m_Session.State!=EditorState::Edit) OnStop(); return; }
    if(m_Session.State!=EditorState::Edit) return;
    if(keys.Pressed(EditorAction::Save)) { SaveScene(); return; }
    if(keys.Pressed(EditorAction::SaveAs)) { SaveScene(true); return; }
    if(keys.Pressed(EditorAction::NewScene)) { NewScene(); return; }
    if(keys.Pressed(EditorAction::OpenScene))
    {
        const auto path=FileDialogs::OpenFile("Finality Scene\0*.fscene\0All Files\0*.*\0\0");
        if(!path.empty()) OpenScene(path); return;
    }
    if(keys.Pressed(EditorAction::Undo)) { if(m_Session.HistoryIndex>0) RestoreHistory(m_Session.HistoryIndex-1); return; }
    if(keys.Pressed(EditorAction::Redo)) { if(m_Session.HistoryIndex+1<m_Session.History.size()) RestoreHistory(m_Session.HistoryIndex+1); return; }
    if(keys.Pressed(EditorAction::Search)) { if(Focused("Hierarchy")) m_FocusSearch="Hierarchy"; else if(m_ContentBrowserFocused) m_FocusSearch="Content Browser"; return; }
    if(m_ContentBrowserFocused) return;
    const bool sceneContext=Focused("Scene") || Focused("Hierarchy") || Focused("Inspector") || m_SceneViewActive;
    if(!sceneContext) return;
    if(keys.Pressed(EditorAction::CreateEmpty)) { RecordHistory(); CreatePrimitive("Empty Entity"); return; }
    if(keys.Pressed(EditorAction::Frame)) { FrameSelected(); return; }
    if(keys.Pressed(EditorAction::Move)) { m_GizmoOperation=0; return; }
    if(keys.Pressed(EditorAction::Rotate)) { m_GizmoOperation=1; return; }
    if(keys.Pressed(EditorAction::Scale)) { m_GizmoOperation=2; return; }
    for(auto action:{EditorAction::Copy,EditorAction::Cut,EditorAction::Paste,EditorAction::Duplicate,EditorAction::Delete,EditorAction::Rename})
        if(keys.Pressed(action)) { ExecuteEntityCommand(action); break; }
}

void FINALITY::EditorLayer::DrawEntityCommandDialogs()
{
    if(m_RenamePending) { ImGui::OpenPopup("Rename Selected Entity"); m_RenamePending=false; }
    if(!ImGui::BeginPopupModal("Rename Selected Entity",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) return;
    auto* scene=SceneManager::GetInstance().GetActiveScene();
    if(!scene || !scene->GetRegistry().valid(m_RenameTarget)) ImGui::CloseCurrentPopup();
    else
    {
        if(ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        EditText("Name",m_RenameBuffer);
        const bool enter=ImGui::IsKeyPressed(ImGuiKey_Enter);
        if(ImGui::Button("Rename") || enter)
        {
            if(!m_RenameBuffer.empty()) { RecordHistory(); scene->GetRegistry().get<TagComponent>(m_RenameTarget).tag=m_RenameBuffer; m_Session.Checkpoint.Request(); }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine(); if(ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void FINALITY::EditorLayer::DrawShortcutSettings()
{
    auto& keys=m_Session.Preferences;
    ImGui::TextWrapped("Click a binding and press the desired keys. Escape cancels. Shortcuts pause while typing or navigating with the right mouse button.");
    for(size_t i=0;i<EditorPreferences::Count;++i)
    {
        const auto& definition=EditorPreferences::Definitions()[i];
        if(!m_SettingsSearch.empty() && !MatchesSearch(definition.Label,m_SettingsSearch) && !MatchesSearch(definition.Scope,m_SettingsSearch) && !MatchesSearch("Keymaps",m_SettingsSearch)) continue;
        const auto action=static_cast<EditorAction>(i); ImGui::PushID(static_cast<int>(i));
        ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(definition.Label); ImGui::SameLine(260);
        if(ImGui::Button(m_ShortcutCapture==i ? "Press keys..." : keys.Label(action).c_str(),{170,0})) { m_ShortcutCapture=static_cast<int>(i); m_ShortcutMessage.clear(); }
        ImGui::SameLine(); if(ImGui::SmallButton("Clear")) { keys.Set(action,0); keys.Save(m_Session.ProjectData.GetDirectory()); }
        ImGui::TextDisabled("%s",definition.Scope); ImGui::PopID();
    }
    if(m_ShortcutCapture>=0)
    {
        if(ImGui::IsKeyPressed(ImGuiKey_Escape)) m_ShortcutCapture=-1;
        else for(int value=ImGuiKey_NamedKey_BEGIN;value<ImGuiKey_GamepadStart;++value)
        {
            if(value>=ImGuiKey_LeftCtrl && value<=ImGuiKey_RightSuper) continue;
            if(!ImGui::IsKeyPressed(static_cast<ImGuiKey>(value),false)) continue;
            const auto chord=value|ImGui::GetIO().KeyMods;
            const auto action=static_cast<EditorAction>(m_ShortcutCapture);
            const int conflict=keys.Conflict(action,chord);
            if(conflict>=0) m_ShortcutMessage=std::string("Already assigned to ")+EditorPreferences::Definitions()[conflict].Label;
            else { keys.Set(action,chord); keys.Save(m_Session.ProjectData.GetDirectory()); m_ShortcutCapture=-1; m_ShortcutMessage.clear(); }
            break;
        }
    }
    if(!m_ShortcutMessage.empty()) ImGui::TextWrapped("%s",m_ShortcutMessage.c_str());
    if(ImGui::Button("Restore Default Keymaps")) { keys={}; keys.Save(m_Session.ProjectData.GetDirectory()); m_ShortcutCapture=-1; }
}
