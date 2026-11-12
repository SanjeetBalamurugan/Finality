#pragma once
#include <Finality.h>
#include "EditorCameraScene.h"

#include "Panels/Console.h"
#include "Panels/PerformancePanel.h"

#include <chrono>
#include <vector>
#include <string>
#include "AssetPicker.h"
#include "Panels/AssetInspector.h"
#include "ProjectScripts.h"

namespace FINALITY
{
	enum class EditorState { Edit, Play, Paused };

	class EditorLayer
	{
	public:
        void Configure(const std::string& initialProject, const std::filesystem::path& engineAssets)
        { m_InitialProject = initialProject; m_EngineAssets = engineAssets; }
		void Init();
		void OnUpdate(float deltaTime);
		void OnImGuiRender();
		void Shutdown();

		void SetSceneFactory(std::function<std::unique_ptr<Scene>()> factory) { m_SceneFactory = factory; }

	private:
		void DrawToolbar();
		void DrawSceneView();
		void DrawGameView();
		void DrawMenuBar();

		void OnPlay();
		void OnPause();
		void OnResume();
		void OnStop();
        void NewScene();
        bool OpenScene(const std::string& path);
        void SaveScene();
        void RecordHistory();
        void RestoreHistory(size_t index);
        void DrawContentBrowser(entt::entity selectedEntityHandle);
        void DrawGizmo(entt::entity selectedEntityHandle);
        bool OpenProject(const std::string& path);
        void DrawProjectManager();
        void DrawProjectSettings();
        void ApplyProjectSettings();
        void DrawCreateMenu();
        void CreatePrimitive(const std::string& name, const std::string& meshReference = {});
        void DrawHierarchy();
        void SetupLayout();
        void InitializeSceneTools();
        void AppendSceneGrid(std::vector<RenderPacket>& queue, Camera& camera);
        void PickSceneEntity(const ImVec2& minimum, const ImVec2& maximum);
        entt::entity m_SelectedEntity = entt::null;

	private:
		std::shared_ptr<RenderTarget> m_SceneTarget;
		std::shared_ptr<RenderTarget> m_GameTarget;

		std::unique_ptr<EditorCameraScene> m_EditorCameraScene;
		bool m_SceneViewActive = false;

		EditorState m_State = EditorState::Edit;
        Project m_Project;
        AssetPicker m_AssetPicker;
        AssetInspector m_AssetInspector;
        ProjectScripts m_ProjectScripts;
        ProjectSettings m_SettingsDraft;
        std::string m_SettingsName, m_SettingsStartup, m_SettingsSearch;
        int m_SettingsCategory = 0;
        bool m_SettingsInitialized = false;
        bool m_ThemePending = false;
        std::string m_ThemeMessage;
        float m_AssetScanClock = 0;
        std::string m_InitialProject;
        std::string m_LayoutPath;
        std::filesystem::path m_EngineAssets;
        std::string m_ProjectName = "New Project";
        std::string m_ProjectLocation;
        std::vector<std::string> m_RecentProjects;
        std::string m_ProjectError;
        std::string m_AssetSearch;
        std::string m_SelectedContentFolder;
        bool m_ContentGrid = true;
        float m_ContentThumbnailSize = 72;
        std::string m_HierarchySearch;
        bool m_ShowProjectManager = true;
        bool m_ShowProjectSettings = false;
        bool m_ShowHierarchy = true;
        bool m_ShowInspector = true;
        bool m_ShowScene = true;
        bool m_ShowGame = true;
        bool m_ShowConsole = true;
        bool m_ShowContentBrowser = true;
        bool m_ShowToolbar = true;
        bool m_ShowPerformance = true;
        std::string m_FocusWindow;
        bool m_UseTemplate = true;
        bool m_LayoutReady = false;
        bool m_LocalGizmo = false;
        bool m_ShowGrid = true;
        std::shared_ptr<Mesh> m_GridMesh;
        std::shared_ptr<Material> m_GridMaterial;
        uint64_t m_SelectedAsset = 0;
        std::vector<std::string> m_History;
        size_t m_HistoryIndex = 0;
        std::string m_ScenePath;
        std::string m_ContentDirectory = "assets";
        float m_ShaderReloadClock = 0.0f;
        int m_GizmoOperation = 0;
		std::function<std::unique_ptr<Scene>()> m_SceneFactory;

		float m_AccumulatedTime = 0.0f;
		float m_UiFps = 0.0f;
		float m_UiAvgFrameTimeMs = 0.0f;
		float m_FrameTimeHistory[120]{};
		int m_HistoryOffset = 0;
		uint32_t m_FrameCount = 0;
		std::chrono::steady_clock::time_point m_LastLogTime;
	};
}
