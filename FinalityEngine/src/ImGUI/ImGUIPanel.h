#pragma once
#include <Core/Core.h>
#include <string>

#include <glm/glm.hpp>

namespace FINALITY
{
	class FAPI ImGUIPanel
	{
	public:
		ImGUIPanel(const std::string& name, bool* p_open = (bool*)0);
		~ImGUIPanel();

		void Text(std::string_view text);

		template<typename... Args>
		void Text(std::string_view fmt, Args&&... args);

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
	private:

	};
}