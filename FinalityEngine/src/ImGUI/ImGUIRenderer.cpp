#include <Core/Core.h>

#include "ImGUIRenderer.h"
#include <Core/Log.h>
#include <Vulkan/VKImGUI/VKImGUIRenderer.h>
#include <Vulkan/VKRenderDevice.h>

#include <backends/imgui_impl_glfw.h>
#include "ImGUIImpl.h"

#include "Fonts/Inter/Inter.h"
#include "Themes/Light01.h"
#include "Themes/Dark01.h"

#include "Themes/Catppuccin/CatppuccinLatte.h"
#include "Themes/Catppuccin/CatppuccinFrappe.h"
#include "Themes/Catppuccin/CatppuccinMacchiato.h"
#include "Themes/Catppuccin/CatppuccinMocha.h"

void FINALITY::ImGUIRenderer::ShutDown()
{
	// TODO: Abstract This to use others than GLFW only
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void FINALITY::ImGUIRenderer::InitImGUI()
{
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // IDK Why but this crashes when i move the panel
	io.DisplaySize.x = (float)m_Specification.width;
	io.DisplaySize.y = (float)m_Specification.height;

	ImGui::GetStyle().FontScaleMain = 1.00f;
	ImFont* interRegular = io.Fonts->AddFontFromMemoryCompressedBase85TTF(Inter_18pt_Regular_compressed_data_base85, 18);
	if (interRegular != nullptr) io.FontDefault = interRegular;

	FINALITY::ApplyCatppuccinMacchiato();

	m_ActiveAPI = this->m_Device->GetActiveApi();

	// TODO: Now for now but this should be changed for android impl i think
	bool InstallGLFWCallbacks = true;
	ImGuiImplGLFW(this->m_ActiveAPI, InstallGLFWCallbacks);
}

std::unique_ptr<FINALITY::ImGUIRenderer> FINALITY::ImGUIRenderer::Create(RenderDevice* device)
{
	RendererAPI currentAPI = device->GetActiveApi();
	WindowSpec spec = device->GetWindowSpec();

	switch (currentAPI)
	{
	case RendererAPI::VULKAN:
	{
		return std::make_unique<VKImGUIRenderer>(spec, device);
		break;
	}
	default:
	{
		FI_CORE_ERROR("Invalid API");
		assert(0);
		break;
	}
	}

	return nullptr;
}
