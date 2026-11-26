/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for EditorSession.h.
 */
#pragma once
#include "ProjectScripts.h"
#include "ExternalTools.h"
#include "ImportProgress.h"
#include "HistoryCheckpoint.h"
#include "EditorPreferences.h"
#include "AssetChangeMonitor.h"

namespace FINALITY
{
    /** @todo TODO: Document EditorState enum semantics, ownership and usage. */
    enum class EditorState
    {
        Edit,
        Play,
        Paused
    };

    /** @todo TODO: Document EditorSession class semantics, ownership and usage. */
    class EditorSession
    {
    public:
        Project ProjectData;
        ProjectScripts Scripts;
        ExternalTools Tools;
        ImportProgress Imports;
        EditorState State = EditorState::Edit;
        entt::entity SelectedEntity = entt::null;
        uint64_t SelectedAsset = 0;
        std::string ScenePath;
        std::vector<std::string> History;
        size_t HistoryIndex = 0;
        HistoryCheckpoint Checkpoint;
        EditorPreferences Preferences;
        AssetChangeMonitor AssetChanges;

        void SelectEntity(entt::entity entity)
        {
            SelectedEntity = entity;
            SelectedAsset = 0;
        }

        void SelectAsset(uint64_t asset)
        {
            SelectedAsset = asset;
            SelectedEntity = entt::null;
        }

        void CloseProjectTools()
        {
            Imports.Clear();
            Tools.CloseProject();
            Scripts.Close();
            SelectEntity(entt::null);
            ScenePath.clear();
            History.clear();
            HistoryIndex = 0;
            Checkpoint.Request();
        }
    };
} // namespace FINALITY
