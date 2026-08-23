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
	panel.SameLine();

	static bool collapse = false;
	panel.Checkbox("Collapse", &collapse);
	panel.SameLine();

	auto rawEntries = Logger::GetConsoleSink()->CopyEntries();

	int infoCount = 0;
	int warnCount = 0;
	int errorCount = 0;

	for (const auto& entry : rawEntries)
	{
		if (entry.Level == spdlog::level::warn)
			warnCount++;
		else if (entry.Level >= spdlog::level::err)
			errorCount++;
		else
			infoCount++;
	}

	static bool showInfo = true;
	static bool showWarn = true;
	static bool showErr = true;
	static std::string searchFilter = "";

	if (searchFilter.capacity() < 256)
		searchFilter.reserve(256);

	std::string infoLabel = fmt::format("Info ({})", infoCount);
	std::string warnLabel = fmt::format("Warn ({})", warnCount);
	std::string errorLabel = fmt::format("Error ({})", errorCount);

	float spacing = panel.GetStyleItemSpacing().x;
	float checkboxPadding = panel.GetFontSize() * 2.5f;

	float infoWidth = panel.CalcTextWidth(infoLabel) + checkboxPadding;
	float warnWidth = panel.CalcTextWidth(warnLabel) + checkboxPadding;
	float errorWidth = panel.CalcTextWidth(errorLabel) + checkboxPadding;

	float filtersWidth = infoWidth + warnWidth + errorWidth + spacing * 2.0f;

	float windowWidth = panel.GetWindowWidth();
	float windowPadding = panel.GetWindowPadding().x;

	float searchWidth = 350.0f;
	float rightControlsWidth = searchWidth + spacing + filtersWidth;
	float rightControlsX = windowWidth - windowPadding - rightControlsWidth;

	float currentX = panel.GetCursorPosX();

	if (currentX < rightControlsX)
	{
		panel.SetCursorPosX(rightControlsX);
	}

	panel.PushItemWidth(searchWidth);
	panel.InputText("##ConsoleSearch", &searchFilter);
	panel.PopItemWidth();
	panel.SameLine();

	panel.Checkbox(infoLabel, &showInfo);
	panel.SameLine();
	panel.Checkbox(warnLabel, &showWarn);
	panel.SameLine();
	panel.Checkbox(errorLabel, &showErr);

	panel.Separator();

	std::vector<LogEntry> displayEntries;
	static size_t lastRawSize = 0;

	bool hasNewEntries = rawEntries.size() > lastRawSize;
	lastRawSize = rawEntries.size();

	std::vector<LogEntry> filteredEntries;

	std::string lowerSearch = searchFilter;

	std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
		});

	for (const auto& entry : rawEntries)
	{
		if (entry.Level == spdlog::level::warn && !showWarn)
			continue;

		if (entry.Level >= spdlog::level::err && !showErr)
			continue;

		if (entry.Level < spdlog::level::warn && !showInfo)
			continue;

		if (!lowerSearch.empty())
		{
			std::string lowerMsg = entry.Message;

			std::transform(lowerMsg.begin(), lowerMsg.end(), lowerMsg.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
				});

			if (lowerMsg.find(lowerSearch) == std::string::npos)
				continue;
		}

		filteredEntries.push_back(entry);
	}

	if (collapse)
	{
		for (const auto& entry : filteredEntries)
		{
			bool found = false;

			for (auto& disp : displayEntries)
			{
				if (disp.RawPayload == entry.RawPayload && disp.Level == entry.Level && disp.LoggerName == entry.LoggerName)
				{
					disp.Count++;
					disp.Message = entry.Message;
					found = true;
					break;
				}
			}

			if (!found)
				displayEntries.push_back(entry);
		}
	}
	else
	{
		displayEntries = filteredEntries;
	}

	static int selectedIndex = -1;

	float footerHeight = panel.GetTextLineHeightWithSpacing() * 5.0f;

	if (panel.BeginChild("ScrollingRegion", glm::vec2(0.0f, -footerHeight), true, PanelFlags_HorizontalScrollbar))
	{
		for (int i = 0; i < static_cast<int>(displayEntries.size()); i++)
		{
			auto& entry = displayEntries[i];

			glm::vec4 color = glm::vec4(1.0f);
			const char* icon = "[i]";

			if (entry.Level == spdlog::level::warn)
			{
				color = glm::vec4(1.0f, 0.82f, 0.21f, 1.0f);
				icon = "[!]";
			}
			else if (entry.Level >= spdlog::level::err)
			{
				color = glm::vec4(0.95f, 0.32f, 0.22f, 1.0f);
				icon = "[X]";
			}

			std::string label;

			if (collapse && entry.Count > 1)
			{
				label = fmt::format("{} ({}) {}", icon, entry.Count, entry.Message);
			}
			else
			{
				label = fmt::format("{} {}", icon, entry.Message);
			}

			bool isSelected = selectedIndex == i;

			panel.PushID(i);
			panel.PushStyleColor(0, color);

			if (panel.Selectable(label, isSelected))
			{
				selectedIndex = i;
			}

			panel.PopStyleColor();
			panel.PopID();
		}

		if (hasNewEntries)
		{
			panel.SetScrollHereY(1.0f);
		}
	}

	panel.EndChild();

	panel.Separator();

	if (panel.BeginChild("DetailsRegion", glm::vec2(0.0f, 0.0f), true, PanelFlags_None))
	{
		if (selectedIndex >= 0 && selectedIndex < static_cast<int>(displayEntries.size()))
		{
			auto& selectedEntry = displayEntries[selectedIndex];

			panel.Text(selectedEntry.Message);

			panel.PushStyleColor(0, glm::vec4(0.55f, 0.55f, 0.55f, 1.0f));

			panel.Text(selectedEntry.stackTrace);

			panel.PopStyleColor();
		}
	}

	panel.EndChild();
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