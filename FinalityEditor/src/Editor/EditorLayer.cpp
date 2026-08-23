#include "EditorLayer.h"

void FINALITY::EditorLayer::Init()
{
	FINALITY::RenderDevice* device = &Application::Get().GetDevice();

	m_SceneTarget = device->CreateRenderTarget(1280, 720);
	m_GameTarget = device->CreateRenderTarget(1280, 720);

	m_SceneTarget->ClearColor = glm::vec4(0.0f, 0.05f, 0.15f, 1.0f);
	device->SetSwapchainClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	device->SetGameRenderTarget(m_GameTarget);

	m_EditorCameraScene = std::make_unique<EditorCameraScene>();
	m_EditorCameraScene->OnInit();

	Renderer::SetDirectRenderingEnabled(false);
	Renderer::SetFrustumCullingEnabled(false);
	SceneManager::GetInstance().SetSimulationEnabled(false);
}

void FINALITY::EditorLayer::OnUpdate(float deltaTime)
{
	if (m_SceneViewActive)
	{
		m_EditorCameraScene->OnUpdate(deltaTime);
	}
}

void FINALITY::EditorLayer::OnImGuiRender()
{
	DrawToolbar();
	DrawMenuBar();
	DrawSceneView();
	DrawGameView();
	DrawConsole();
}

void FINALITY::EditorLayer::DrawToolbar()
{
	//FINALITY::ImGUIPanel panel("Toolbar", nullptr, FINALITY::PanelFlags_NoTitleBar | FINALITY::PanelFlags_NoResize | FINALITY::PanelFlags_NoMove);
	FINALITY::ImGUIPanel panel("Toolbar");

	if (panel.Button("Play") && m_State == EditorState::Edit)
	{
		OnPlay();
		m_State = EditorState::Play;
	}
	panel.SameLine();

	std::string_view pauseLabel = (m_State == EditorState::Paused) ? "Resume" : "Pause";
	if (panel.Button(pauseLabel))
	{
		if (m_State == EditorState::Play) OnPause();
		else if (m_State == EditorState::Paused) OnResume();
	}
	panel.SameLine();

	if (panel.Button("Stop") && m_State != EditorState::Edit)
	{
		OnStop();
		m_State = EditorState::Edit;
	}
}

void FINALITY::EditorLayer::DrawSceneView()
{
	FINALITY::ImGUIPanel panel("Scene");

	m_SceneViewActive = panel.IsFocused() || panel.IsHovered();

	glm::vec2 panelSize = panel.GetContentRegionAvail();
	if (panelSize.x < 1.0f || panelSize.y < 1.0f) return;

	uint32_t width = static_cast<uint32_t>(panelSize.x);
	uint32_t height = static_cast<uint32_t>(panelSize.y);

	FINALITY::RenderDevice* device = &Application::Get().GetDevice();

	device->ResizeRenderTarget(m_SceneTarget, width, height);

	Camera* editorCam = m_EditorCameraScene->GetCamera();
	editorCam->SetViewportSize(static_cast<float>(width), static_cast<float>(height));

	device->UpdateRenderTargetCamera(m_SceneTarget, editorCam);
	device->RenderSceneToTarget(m_SceneTarget, Renderer::GetRenderQueue());

	void* textureHandle = device->GetRenderTargetTextureHandle(m_SceneTarget);
	panel.Image(textureHandle, panelSize);
}

void FINALITY::EditorLayer::DrawGameView()
{
	FINALITY::ImGUIPanel panel("Game");

	glm::vec2 panelSize = panel.GetContentRegionAvail();
	if (panelSize.x < 1.0f || panelSize.y < 1.0f) return;

	uint32_t width = static_cast<uint32_t>(panelSize.x);
	uint32_t height = static_cast<uint32_t>(panelSize.y);

	FINALITY::RenderDevice* device = &Application::Get().GetDevice();
	device->ResizeRenderTarget(m_GameTarget, width, height);

	Camera* runtimeCamera = Renderer::GetActiveCamera();
	if (runtimeCamera)
	{
		runtimeCamera->SetViewportSize(static_cast<float>(width), static_cast<float>(height));
		device->UpdateRenderTargetCamera(m_GameTarget, runtimeCamera);
		device->RenderSceneToTarget(m_GameTarget, Renderer::GetRenderQueue());
		void* textureHandle = device->GetRenderTargetTextureHandle(m_GameTarget);
		panel.Image(textureHandle, panelSize);
	}
}

void FINALITY::EditorLayer::DrawMenuBar()
{
	if (!FINALITY::ImGUIPanel::BeginMainMenuBar()) return;

	if (FINALITY::ImGUIPanel::BeginMenu("File"))
	{
		FINALITY::ImGUIPanel::MenuItem("New Scene");
		FINALITY::ImGUIPanel::MenuItem("Save Scene");
		if (FINALITY::ImGUIPanel::MenuItem("Exit")) Application::Get().RequestClose();
		FINALITY::ImGUIPanel::EndMenu();
	}

	if (FINALITY::ImGUIPanel::BeginMenu("Edit"))
	{
		FINALITY::ImGUIPanel::MenuItem("Undo");
		FINALITY::ImGUIPanel::MenuItem("Redo");
		FINALITY::ImGUIPanel::EndMenu();
	}

	if (FINALITY::ImGUIPanel::BeginMenu("Help"))
	{
		FINALITY::ImGUIPanel::MenuItem("About");
		FINALITY::ImGUIPanel::EndMenu();
	}

	FINALITY::ImGUIPanel::EndMainMenuBar();
}

void FINALITY::EditorLayer::DrawConsole()
{
	FINALITY::ImGUIPanel panel("Console");

	if (panel.Button("Clear"))
	{
		Logger::GetConsoleSink()->Clear();
	}

	auto entries = Logger::GetConsoleSink()->CopyEntries();
	std::string countMsg = "Entry count: " + std::to_string(entries.size());
	panel.Text(countMsg);
	for (auto& entry : entries)
	{
		glm::vec4 color = glm::vec4(1.0f);
		if (entry.Level == spdlog::level::warn) color = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);
		else if (entry.Level >= spdlog::level::err) color = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);

		panel.TextColored(color, entry.Message);
	}
}

void FINALITY::EditorLayer::OnPlay()
{
	m_State = EditorState::Play;
	SceneManager::GetInstance().SetSimulationEnabled(true);
}

void FINALITY::EditorLayer::OnPause()
{
	m_State = EditorState::Paused;
	SceneManager::GetInstance().SetSimulationEnabled(false);
}

void FINALITY::EditorLayer::OnResume()
{
	m_State = EditorState::Play;
	SceneManager::GetInstance().SetSimulationEnabled(true);
}

void FINALITY::EditorLayer::OnStop()
{
	m_State = EditorState::Edit;
	SceneManager::GetInstance().SetSimulationEnabled(false);

	if (m_SceneFactory)
	{
		SceneManager::GetInstance().SetScene(m_SceneFactory());
		SceneManager::GetInstance().ChangeScene();
	}
}

void FINALITY::EditorLayer::Shutdown()
{
	Renderer::SetFrustumCullingEnabled(true);
	Renderer::SetDirectRenderingEnabled(true);

	FINALITY::RenderDevice* device = &Application::Get().GetDevice();
	device->WaitIdle();

	if (m_EditorCameraScene)
	{
		m_EditorCameraScene->OnDestroy();
		m_EditorCameraScene.reset();
	}

	device->SetGameRenderTarget(nullptr);

	if (m_SceneTarget) { device->DestroyRenderTargetGpuResources(*m_SceneTarget); m_SceneTarget.reset(); }
	if (m_GameTarget) { device->DestroyRenderTargetGpuResources(*m_GameTarget); m_GameTarget.reset(); }
}