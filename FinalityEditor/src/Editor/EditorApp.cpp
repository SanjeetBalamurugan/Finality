#include "EditorApp.h"
#include "MainScene.h"

void FINALITY::EditorApp::Init()
{
	m_EditorLayer = std::make_unique<EditorLayer>();
	m_EditorLayer->Init();

	m_EditorLayer->SetSceneFactory([]() { return std::make_unique<MainScene>(); });

	auto next = std::make_unique<MainScene>();
	FINALITY::SceneManager::GetInstance().SetScene(std::move(next));
	FINALITY::SceneManager::GetInstance().ChangeScene();
}

void FINALITY::EditorApp::Update(float deltaTime)
{
	m_EditorLayer->OnUpdate(deltaTime);
	m_EditorLayer->OnImGuiRender();
}

void FINALITY::EditorApp::Destroy()
{
	if (m_EditorLayer)
	{
		m_EditorLayer->Shutdown();
	}
	m_EditorLayer.reset();
}