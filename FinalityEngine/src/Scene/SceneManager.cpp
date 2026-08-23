#include "SceneManager.h"

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
	if (m_NextScene) m_NextScene->OnDestroy();
}

void FINALITY::SceneManager::SetScene(std::unique_ptr<Scene> scene)
{
	m_NextScene = std::move(scene);
}

void FINALITY::SceneManager::ChangeScene()
{
	m_CurrentScene->OnDestroy();
	m_CurrentScene = std::move(m_NextScene);
	m_CurrentScene->OnInit();
}
