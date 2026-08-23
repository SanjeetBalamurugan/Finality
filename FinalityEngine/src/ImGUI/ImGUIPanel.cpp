#include "ImGUIPanel.h"

#include <imgui.h>

FINALITY::ImGUIPanel::ImGUIPanel(const std::string& name, bool* p_open, PanelFlags flags)
{
	ImGui::Begin(name.c_str(), p_open, static_cast<ImGuiWindowFlags>(flags));
}

FINALITY::ImGUIPanel::~ImGUIPanel()
{
	ImGui::End();
}

void FINALITY::ImGUIPanel::Text(std::string_view text)
{
    ImGui::TextUnformatted(text.data(), text.data() + text.size());
}

void FINALITY::ImGUIPanel::Separator()
{
	ImGui::Separator();
}

void FINALITY::ImGUIPanel::Text(std::string_view fmt, float value)
{
	ImGui::Text(fmt.data(), value);
}

void FINALITY::ImGUIPanel::Text(std::string_view fmt, unsigned int value)
{
	ImGui::Text(fmt.data(), value);
}

void FINALITY::ImGUIPanel::Text(std::string_view fmt, unsigned int val1, float val2)
{
	ImGui::Text(fmt.data(), val1, val2);
}

void FINALITY::ImGUIPanel::TextColored(const glm::vec4& col, std::string_view text)
{
	ImGui::TextColored(ImVec4(col.r, col.g, col.b, col.a), "%.*s", (int)text.size(), text.data());
}

bool FINALITY::ImGUIPanel::Checkbox(std::string_view label, bool* v)
{
	return ImGui::Checkbox(label.data(), v);
}

bool FINALITY::ImGUIPanel::RadioButton(std::string_view label, int* v, int v_button)
{
	return ImGui::RadioButton(label.data(), v, v_button);
}

void FINALITY::ImGUIPanel::SameLine(float offset_from_start_x, float spacing)
{
	ImGui::SameLine(offset_from_start_x, spacing);
}

bool FINALITY::ImGUIPanel::SliderFloat(std::string_view label, float* v, float v_min, float v_max, std::string_view format)
{
	return ImGui::SliderFloat(label.data(), v, v_min, v_max, format.data());
}

bool FINALITY::ImGUIPanel::DragInt(std::string_view label, int* v, float v_speed, int v_min, int v_max)
{
	return ImGui::DragInt(label.data(), v, v_speed, v_min, v_max);
}

bool FINALITY::ImGUIPanel::Combo(std::string_view label, int* current_item, const char* const items[], int items_count)
{
	return ImGui::Combo(label.data(), current_item, items, items_count);
}

bool FINALITY::ImGUIPanel::ColorEdit4(std::string_view label, float col[4])
{
	return ImGui::ColorEdit4(label.data(), col);
}

bool FINALITY::ImGUIPanel::Button(std::string_view label, const glm::vec2& size)
{
	return ImGui::Button(label.data(), ImVec2(size.x, size.y));
}

bool FINALITY::ImGUIPanel::BeginTabBar(std::string_view str_id)
{
	return ImGui::BeginTabBar(str_id.data());
}

void FINALITY::ImGUIPanel::EndTabBar()
{
	ImGui::EndTabBar();
}

bool FINALITY::ImGUIPanel::BeginTabItem(std::string_view label, bool* p_open)
{
	return ImGui::BeginTabItem(label.data(), p_open);
}

void FINALITY::ImGUIPanel::EndTabItem()
{
	ImGui::EndTabItem();
}

bool FINALITY::ImGUIPanel::TreeNode(std::string_view label)
{
	return ImGui::TreeNode(label.data());
}

void FINALITY::ImGUIPanel::TreePop()
{
	ImGui::TreePop();
}

void FINALITY::ImGUIPanel::PlotLines(
	std::string_view label,
	const float* values,
	int values_count,
	int values_offset,
	std::string_view overlay_text,
	float scale_min,
	float scale_max,
	glm::vec2 graph_size)
{
	ImVec2 im_size(graph_size.x, graph_size.y);

	ImGui::PlotLines(
		label.data(),
		values,
		values_count,
		values_offset,
		overlay_text.empty() ? nullptr : overlay_text.data(),
		scale_min,
		scale_max,
		im_size
	);
}

void FINALITY::ImGUIPanel::SetNextWindowPos(const glm::vec2& pos, PanelCondition cond, const glm::vec2& pivot)
{
	ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), static_cast<ImGuiCond>(cond), ImVec2(pivot.x, pivot.y));
}

void FINALITY::ImGUIPanel::SetNextWindowBgAlpha(float alpha)
{
	ImGui::SetNextWindowBgAlpha(alpha);
}

void FINALITY::ImGUIPanel::Image(void* textureHandle, const glm::vec2& size, const glm::vec2& uv0, const glm::vec2& uv1)
{
	ImGui::Image((ImTextureID)textureHandle, ImVec2(size.x, size.y), ImVec2(uv0.x, uv0.y), ImVec2(uv1.x, uv1.y));
}

glm::vec2 FINALITY::ImGUIPanel::GetContentRegionAvail()
{
	ImVec2 avail = ImGui::GetContentRegionAvail();
	return glm::vec2(avail.x, avail.y);
}

bool FINALITY::ImGUIPanel::IsFocused()
{
	return ImGui::IsWindowFocused();
}

bool FINALITY::ImGUIPanel::IsHovered()
{
	return ImGui::IsWindowHovered();
}

bool FINALITY::ImGUIPanel::BeginMainMenuBar()
{
	return ImGui::BeginMainMenuBar();
}

void FINALITY::ImGUIPanel::EndMainMenuBar()
{
	ImGui::EndMainMenuBar();
}

bool FINALITY::ImGUIPanel::BeginMenu(std::string_view label)
{
	return ImGui::BeginMenu(label.data());
}

void FINALITY::ImGUIPanel::EndMenu()
{
	ImGui::EndMenu();
}

bool FINALITY::ImGUIPanel::MenuItem(std::string_view label, std::string_view shortcut)
{
	return ImGui::MenuItem(label.data(), shortcut.empty() ? nullptr : shortcut.data());
}