#include "Application.h"
#include <iostream>
#include <chrono>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "GLFWWindowImpl.h"

// Vulkan Specific
#include <Vulkan/VKRenderDevice.h>
//

#include "Log.h"
#include <Scene/SceneManager.h>
#include "RenderCommand.h"
#include <Renderer/Renderer.h>
#include <Events/Input.h>
#include <Events/Mouse.h>
#include <Assets/AssetManager.h>

#include <ImGUI/ImGUIRenderer.h>

#define GLM_FORCE_PURE  
#define GLM_ENABLE_EXPERIMENTAL

FINALITY::RendererAPI FINALITY::Application::s_CurrentAPI = RendererAPI::NONE;

FINALITY::Application::Application() = default;
FINALITY::Application::~Application() = default;

void FINALITY::Application::Initialize(const RendererAPI& api, const WindowSpec& spec, std::unique_ptr<Game> game)
{
	s_Instance = this;
	Logger::Init();

	FI_CORE_INFO("FINALITY ENGINE");

	if (!glfwInit())
	{
		FI_CORE_ERROR("GLFW Cannot Initialise!!");
		std::exit(EXIT_FAILURE);
	}

	if (!glfwVulkanSupported() && api == RendererAPI::VULKAN)
	{
		FI_CORE_ERROR("VULKAN is not supported on this system");
		std::exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	switch (api)
	{
	case RendererAPI::VULKAN:
		m_RenderDevice = std::make_unique<VKRenderDevice>();
		break;
	default:
		FI_CORE_ERROR("UNKNOWN API!!!");
		std::exit(EXIT_FAILURE);
		break;
	}

	s_CurrentAPI = api;

	m_CurrentGame = std::move(game);

	m_Window = std::make_unique<GLFWWindowImpl>();
	m_Window->Initialize(spec);

	m_RenderDevice->SetWindowSpec(spec);
	m_RenderDevice->Initialize(m_Window->GetNativeHandles());
	FINALITY::RenderCommand::Init(m_RenderDevice.get());

	FINALITY::Renderer::Initialize(m_RenderDevice.get());

	m_ImGUIRenderer = ImGUIRenderer::Create(m_RenderDevice.get());
	m_ImGUIRenderer->Initialize();

	m_CurrentGame->Init();

	FINALITY::AssetManager::Initialize();
	SceneManager::GetInstance().Initialize();

	m_Running = true;
}

void FINALITY::Application::Update()
{
	auto lastFrameTime = std::chrono::high_resolution_clock::now();
	float deltaTime = 0.0f;

	while (!m_Window->ShouldClose() && m_Running)
	{
		auto currentFrameTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> elapsed = currentFrameTime - lastFrameTime;
		deltaTime = elapsed.count();

		lastFrameTime = currentFrameTime;

		m_Window->Update();

		m_RenderDevice->BeginFrame();

		m_ImGUIRenderer->Update();
		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

		// Frame logic like draw calls here
		m_CurrentGame->Update(deltaTime);
		SceneManager::GetInstance().Update(deltaTime);

		ImGui::Render();
		m_RenderDevice->EndFrame();
		m_RenderDevice->PresentFrame();

		Input::ClearFrameStates();
		Mouse::ClearFrameStates();
	}

	m_Running = false;
}

void FINALITY::Application::Shutdown()
{
	SceneManager::GetInstance().Shutdown();
	FINALITY::AssetManager::Shutdown();
	FINALITY::Renderer::Shutdown();

	m_ImGUIRenderer->Destroy();

	if (m_CurrentGame) m_CurrentGame->Destroy();
	if (m_Window)
	{
		m_Window->Shutdown();
		m_Window.reset();
	}
	
	if (m_RenderDevice)
	{
		m_RenderDevice->Shutdown();
		m_RenderDevice.reset();
	}

	if (!m_Running) glfwTerminate();
}

FINALITY::ImGUIRenderer* FINALITY::Application::GetImGUIRenderer()
{
	return m_ImGUIRenderer.get();
}
