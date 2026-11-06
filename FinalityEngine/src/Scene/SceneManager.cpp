#include "SceneManager.h"
#include <Renderer/Renderer.h>

void FINALITY::SceneManager::Initialize() const
{
	m_CurrentScene->OnInit();
}

void FINALITY::SceneManager::Update(float ts) const
{
	if (m_SimulationEnabled)
	{
		m_CurrentScene->OnUpdate(ts);
	}

	m_CurrentScene->OnRender();
}

void FINALITY::SceneManager::Shutdown() const
{
	m_CurrentScene->OnDestroy();
    if (m_EditorScene) m_EditorScene->OnDestroy();
	if (m_NextScene) m_NextScene->OnDestroy();
}

void FINALITY::SceneManager::SetScene(std::unique_ptr<Scene> scene)
{
	m_NextScene = std::move(scene);
}

void FINALITY::SceneManager::ChangeScene()
{
    if (!m_NextScene || IsRuntimeSceneActive()) return;
    Renderer::SetActiveCamera(nullptr);
	m_CurrentScene->OnDestroy();
	m_CurrentScene = std::move(m_NextScene);
	m_CurrentScene->OnInit();
}

bool FINALITY::SceneManager::EnterRuntime(std::unique_ptr<Scene> scene)
{
    if (!scene || m_EditorScene) return false;
    Renderer::SetActiveCamera(nullptr);
    m_EditorScene = std::move(m_CurrentScene);
    m_CurrentScene = std::move(scene);
    m_CurrentScene->OnInit();
    m_SimulationEnabled = true;
    return true;
}

void FINALITY::SceneManager::ExitRuntime()
{
    if (!m_EditorScene) return;
    m_SimulationEnabled = false;
    Renderer::SetActiveCamera(nullptr);
    m_CurrentScene->OnDestroy();
    m_CurrentScene = std::move(m_EditorScene);
}
