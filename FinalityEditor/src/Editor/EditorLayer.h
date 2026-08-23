#pragma once
#include <Finality.h>
#include "EditorCameraScene.h"

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
		void DrawConsole();

		void OnPlay();
		void OnPause();
		void OnResume();
		void OnStop();

	private:
		std::shared_ptr<RenderTarget> m_SceneTarget;
		std::shared_ptr<RenderTarget> m_GameTarget;

		std::unique_ptr<EditorCameraScene> m_EditorCameraScene;
		bool m_SceneViewActive = false;

		EditorState m_State = EditorState::Edit;
		std::function<std::unique_ptr<Scene>()> m_SceneFactory;
	};
}