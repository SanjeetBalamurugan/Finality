#pragma once
#include <Core/Core.h>
#include <string>

#include <glm/glm.hpp>

namespace FINALITY
{
	enum PanelColor_
	{
		PanelColor_Text = 0,
		PanelColor_TextDisabled = 1,
		PanelColor_WindowBg = 2,
		PanelColor_ChildBg = 3,
		PanelColor_PopupBg = 4,
		PanelColor_Border = 5,
		PanelColor_BorderShadow = 6,
		PanelColor_FrameBg = 7,
		PanelColor_FrameBgHovered = 8,
		PanelColor_FrameBgActive = 9,
		PanelColor_TitleBg = 10,
		PanelColor_TitleBgActive = 11,
		PanelColor_TitleBgCollapsed = 12,
		PanelColor_MenuBarBg = 13,
		PanelColor_ScrollbarBg = 14,
		PanelColor_ScrollbarGrab = 15,
		PanelColor_ScrollbarGrabHovered = 16,
		PanelColor_ScrollbarGrabActive = 17,
		PanelColor_CheckMark = 18,
		PanelColor_SliderGrab = 19,
		PanelColor_SliderGrabActive = 20,
		PanelColor_Button = 21,
		PanelColor_ButtonHovered = 22,
		PanelColor_ButtonActive = 23,
		PanelColor_Header = 24,
		PanelColor_HeaderHovered = 25,
		PanelColor_HeaderActive = 26,
		PanelColor_Separator = 27,
		PanelColor_SeparatorHovered = 28,
		PanelColor_SeparatorActive = 29,
		PanelColor_ResizeGrip = 30,
		PanelColor_ResizeGripHovered = 31,
		PanelColor_ResizeGripActive = 32,
		PanelColor_InputTextCursor = 33,
		PanelColor_TabHovered = 34,
		PanelColor_Tab = 35,
		PanelColor_TabSelected = 36,
		PanelColor_TabSelectedOverline = 37,
		PanelColor_TabDimmed = 38,
		PanelColor_TabDimmedSelected = 39,
		PanelColor_TabDimmedSelectedOverline = 40,
		PanelColor_DockingPreview = 41,
		PanelColor_DockingEmptyBg = 42,
		PanelColor_PlotLines = 43,
		PanelColor_PlotLinesHovered = 44,
		PanelColor_PlotHistogram = 45,
		PanelColor_PlotHistogramHovered = 46,
		PanelColor_TableHeaderBg = 47,
		PanelColor_TableBorderStrong = 48,
		PanelColor_TableBorderLight = 49,
		PanelColor_TableRowBg = 50,
		PanelColor_TableRowBgAlt = 51,
		PanelColor_TextLink = 52,
		PanelColor_TextSelectedBg = 53,
		PanelColor_DragDropTarget = 54,
		PanelColor_NavCursor = 55,
		PanelColor_NavWindowingHighlight = 56,
		PanelColor_NavWindowingDimBg = 57,
		PanelColor_ModalWindowDimBg = 58
	};

	using PanelColor = int;

	enum PanelFlags_
	{
		PanelFlags_None = 0,
		PanelFlags_NoTitleBar = 1 << 0,
		PanelFlags_NoResize = 1 << 1,
		PanelFlags_NoMove = 1 << 2,
		PanelFlags_NoScrollbar = 1 << 3,
		PanelFlags_HorizontalScrollbar = 1 << 5,
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

	enum PanelPopupFlags_
	{
		PanelPopupFlags_None = 0,
		PanelPopupFlags_MouseButtonLeft = 0,
		PanelPopupFlags_MouseButtonRight = 1,
		PanelPopupFlags_MouseButtonMiddle = 2,
		PanelPopupFlags_MouseButtonMask_ = 0x1F,
		PanelPopupFlags_MouseButtonDefault_ = 1,
		PanelPopupFlags_NoOpenOverExistingPopup = 1 << 5,
		PanelPopupFlags_NoOpenOverItems = 1 << 6,
		PanelPopupFlags_AnyPopupId = 1 << 7,
		PanelPopupFlags_AnyPopupLevel = 1 << 8,
		PanelPopupFlags_AnyPopup = PanelPopupFlags_AnyPopupId | PanelPopupFlags_AnyPopupLevel
	};
	using PanelPopupFlags = int;

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

		bool BeginChild(std::string_view str_id, const glm::vec2& size = glm::vec2(0.0f, 0.0f), bool border = false, PanelFlags flags = 0);
		void EndChild();
		bool Selectable(std::string_view label, bool selected = false);
		void PushStyleColor(int idx, const glm::vec4& col);
		void PopStyleColor(int count = 1);
		float GetTextLineHeightWithSpacing();

		void SetScrollHereY(float center_y_ratio = 0.5f);
		bool InputText(std::string_view label, std::string* str, PanelFlags flags = 0);

		float GetFontSize();
		glm::vec2 GetStyleItemSpacing();
		void PushItemWidth(float item_width);
		void PopItemWidth();
		float GetCursorPosX();

		void PushID(int int_id);
		void PushID(std::string_view str_id);
		void PopID();

		float GetWindowWidth();
		float GetWindowHeight();

		void SetCursorPosX(float x);
		float CalcTextWidth(const std::string& text);
		bool TreeNode(std::string_view label, bool selected);
		bool IsItemClicked(int mouseButton = 0);

		void Indent(float width = 0.0f);
		void Unindent(float width = 0.0f);
		void Spacing();

		glm::vec2 GetWindowPadding();

		void OpenPopup(std::string_view str_id, PanelPopupFlags popup_flags = 0);
		bool BeginPopup(std::string_view str_id, PanelFlags flags = 0);
		void EndPopup();

		bool IsAnyItemHovered();
		bool IsItemHovered();
		bool IsMouseClicked(int a);

		void CloseCurrentPopup();
	private:
	};
}
