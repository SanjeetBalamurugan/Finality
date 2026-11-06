#pragma once
#include <Finality.h>
#include "EditorCameraScene.h"

#include "Panels/Console.h"
#include "Panels/PerformancePanel.h"
#include "Panels/SceneHierarchyPanel.h"

#include <chrono>
#include <vector>
#include <string>

namespace FINALITY
{
	enum class EditorState { Edit, Play, Paused };

	class EditorLayer
	{
	public:
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
        entt::entity m_SelectedEntity = entt::null;

	private:
		std::shared_ptr<RenderTarget> m_SceneTarget;
		std::shared_ptr<RenderTarget> m_GameTarget;

		std::unique_ptr<EditorCameraScene> m_EditorCameraScene;
		bool m_SceneViewActive = false;

		EditorState m_State = EditorState::Edit;
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