#pragma once
#include <Core/Core.h>
#include <string>

#include <glm/glm.hpp>

namespace FINALITY
{
	enum PanelFlags_
	{
		PanelFlags_None = 0,
		PanelFlags_NoTitleBar = 1 << 0,
		PanelFlags_NoResize = 1 << 1,
		PanelFlags_NoMove = 1 << 2,
		PanelFlags_NoScrollbar = 1 << 3,
		PanelFlags_AlwaysAutoResize = 1 << 6,
		PanelFlags_NoBackground = 1 << 7,
		PanelFlags_NoSavedSettings = 1 << 8,
		PanelFlags_NoFocusOnAppearing = 1 << 11,
		PanelFlags_NoNav = 1 << 14,
		PanelFlags_NoDecoration = PanelFlags_NoTitleBar | PanelFlags_NoResize | PanelFlags_NoScrollbar
	};
	using PanelFlags = int;

	enum PanelCondition_
	{
		PanelCondition_Always = 1 << 0,
		PanelCondition_Once = 1 << 1,
		PanelCondition_FirstUseEver = 1 << 2,
		PanelCondition_Appearing = 1 << 3
	};
	using PanelCondition = int;

	class FAPI ImGUIPanel
	{
	public:
		ImGUIPanel(const std::string& name, bool* p_open = (bool*)0, PanelFlags flags = 0);
		~ImGUIPanel();

		void Text(std::string_view text);
		void Text(std::string_view fmt, float value);
		void Text(std::string_view fmt, unsigned int value);
		void Text(std::string_view fmt, unsigned int val1, float val2);

		void Separator();

		void TextColored(const glm::vec4& col, std::string_view text);
		bool Checkbox(std::string_view label, bool* v);
		bool RadioButton(std::string_view label, int* v, int v_button);
		void SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f);
		bool SliderFloat(std::string_view label, float* v, float v_min, float v_max, std::string_view format = "%.3f");
		bool DragInt(std::string_view label, int* v, float v_speed = 1.0f, int v_min = 0, int v_max = 0);
		bool Combo(std::string_view label, int* current_item, const char* const items[], int items_count);
		bool ColorEdit4(std::string_view label, float col[4]);
		bool Button(std::string_view label, const glm::vec2& size = glm::vec2(0, 0));

		bool BeginTabBar(std::string_view str_id);
		void EndTabBar();

		bool BeginTabItem(std::string_view label, bool* p_open = (bool*)0);
		void EndTabItem();

		bool TreeNode(std::string_view label);
		void TreePop();

		void PlotLines(
			std::string_view label,
			const float* values,
			int values_count,
			int values_offset = 0,
			std::string_view overlay_text = "",
			float scale_min = FLT_MAX,
			float scale_max = FLT_MAX,
			glm::vec2 graph_size = glm::vec2(0.0f, 0.0f)
		);

		static void SetNextWindowPos(const glm::vec2& pos, PanelCondition cond = PanelCondition_Always, const glm::vec2& pivot = glm::vec2(0.0f, 0.0f));
		static void SetNextWindowBgAlpha(float alpha);

		void Image(void* textureHandle, const glm::vec2& size, const glm::vec2& uv0 = glm::vec2(0.0f, 0.0f), const glm::vec2& uv1 = glm::vec2(1.0f, 1.0f));
		glm::vec2 GetContentRegionAvail();

		bool IsFocused();
		bool IsHovered();

		static bool BeginMainMenuBar();
		static void EndMainMenuBar();
		static bool BeginMenu(std::string_view label);
		static void EndMenu();
		static bool MenuItem(std::string_view label, std::string_view shortcut = "");
	private:
	};
}