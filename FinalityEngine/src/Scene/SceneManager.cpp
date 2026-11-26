#include "SceneManager.h"
#include <Renderer/Renderer.h>

void FINALITY::SceneManager::Initialize() const
{
    m_CurrentScene->OnInit();
}

void FINALITY::SceneManager::Update(float ts)
{
    if (m_ChangeRequested)
    {
        ChangeScene();
    }
    m_Updating = true;
    try
    {
        if (m_SimulationEnabled)
        {
            m_CurrentScene->OnUpdate(ts);
        }
        m_CurrentScene->OnRender();
    }
    catch (...)
    {
        m_Updating = false;
        throw;
    }
    m_Updating = false;
    if (m_ExitRequested)
    {
        m_ExitRequested = false;
        ExitRuntime();
    }
    if (m_ChangeRequested)
    {
        ChangeScene();
    }
}

void FINALITY::SceneManager::Shutdown()
{
    m_Applying = true;

    struct Reset
    {
        bool& Flag;

        ~Reset()
        {
            Flag = false;
        }
    } reset{ m_Applying };

    auto pending = std::move(m_NextScene);
    m_CurrentScene->OnDestroy();
    if (m_EditorScene)
    {
        m_EditorScene->OnDestroy();
    }
    if (pending)
    {
        pending->OnDestroy();
    }
    m_NextScene.reset();
    m_ChangeRequested = m_ExitRequested = false;
}

void FINALITY::SceneManager::SetScene(std::unique_ptr<Scene> scene)
{
    m_NextScene = std::move(scene);
}

void FINALITY::SceneManager::ChangeScene()
{
    if (!m_NextScene)
    {
        return;
    }
    if (m_Updating || m_Applying)
    {
        m_ChangeRequested = true;
        return;
    }
    auto next = std::move(m_NextScene);
    m_ChangeRequested = false;
    m_Applying = true;

    struct Reset
    {
        bool& Flag;

        ~Reset()
        {
            Flag = false;
        }
    } reset{ m_Applying };

    Renderer::SetActiveCamera(nullptr);
    Renderer::BeginScene();
    m_CurrentScene->OnDestroy();
    m_CurrentScene = std::move(next);
    m_CurrentScene->OnInit();
}

bool FINALITY::SceneManager::EnterRuntime(std::unique_ptr<Scene> scene)
{
    if (!scene || m_EditorScene || m_Updating || m_Applying)
    {
        return false;
    }
    Renderer::SetActiveCamera(nullptr);
    m_EditorScene = std::move(m_CurrentScene);
    m_CurrentScene = std::move(scene);
    m_CurrentScene->OnInit();
    m_SimulationEnabled = true;
    return true;
}

void FINALITY::SceneManager::ExitRuntime()
{
    if (m_Updating || m_Applying)
    {
        m_ExitRequested = true;
        return;
    }
    if (!m_EditorScene)
    {
        return;
    }
    m_Applying = true;

    struct Reset
    {
        bool& Flag;

        ~Reset()
        {
            Flag = false;
        }
    } reset{ m_Applying };

    m_SimulationEnabled = false;
    Renderer::SetActiveCamera(nullptr);
    m_CurrentScene->OnDestroy();
    m_CurrentScene = std::move(m_EditorScene);
}
