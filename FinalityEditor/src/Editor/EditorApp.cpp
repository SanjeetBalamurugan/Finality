#include "EditorApp.h"
#include <MainScene.h>

void FINALITY::EditorApp::Init()
{
    m_EditorLayer = std::make_unique<EditorLayer>();
    m_EditorLayer->Configure(m_InitialProject, m_EngineAssets);
    m_EditorLayer->Init();
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
