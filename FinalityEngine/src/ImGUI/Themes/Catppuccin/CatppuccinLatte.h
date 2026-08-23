#pragma once
#include <imgui.h>

namespace FINALITY
{
    static void ApplyCatppuccinLatte() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_WindowBg] = ImVec4(0.937f, 0.945f, 0.961f, 1.000f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.902f, 0.914f, 0.937f, 1.000f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.863f, 0.878f, 0.910f, 0.980f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.902f, 0.914f, 0.937f, 1.000f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.863f, 0.878f, 0.910f, 1.000f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.800f, 0.824f, 0.871f, 1.000f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.863f, 0.878f, 0.910f, 1.000f);
        colors[ImGuiCol_Header] = ImVec4(0.447f, 0.529f, 0.992f, 1.000f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.118f, 0.400f, 0.961f, 1.000f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.090f, 0.357f, 0.922f, 1.000f);

        colors[ImGuiCol_Button] = ImVec4(0.737f, 0.753f, 0.816f, 1.000f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.675f, 0.694f, 0.761f, 1.000f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.612f, 0.631f, 0.706f, 1.000f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.118f, 0.400f, 0.961f, 1.000f);

        colors[ImGuiCol_FrameBg] = ImVec4(0.800f, 0.824f, 0.871f, 1.000f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.737f, 0.753f, 0.816f, 1.000f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.675f, 0.694f, 0.761f, 1.000f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.447f, 0.529f, 0.992f, 1.000f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.118f, 0.400f, 0.961f, 1.000f);

        colors[ImGuiCol_Tab] = ImVec4(0.902f, 0.914f, 0.937f, 1.000f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.800f, 0.824f, 0.871f, 1.000f);
        colors[ImGuiCol_TabActive] = ImVec4(0.937f, 0.945f, 0.961f, 1.000f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.902f, 0.914f, 0.937f, 1.000f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.937f, 0.945f, 0.961f, 1.000f);

        colors[ImGuiCol_Border] = ImVec4(0.800f, 0.824f, 0.871f, 1.000f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
        colors[ImGuiCol_Separator] = ImVec4(0.800f, 0.824f, 0.871f, 1.000f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.447f, 0.529f, 0.992f, 1.000f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.118f, 0.400f, 0.961f, 1.000f);

        colors[ImGuiCol_Text] = ImVec4(0.298f, 0.310f, 0.412f, 1.000f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.424f, 0.435f, 0.529f, 1.000f);

        style.WindowPadding = ImVec2(6.0f, 6.0f);
        style.WindowRounding = 4.0f;
        style.FrameRounding = 4.0f;
        style.GrabRounding = 3.0f;
        style.TabRounding = 4.0f;
        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 1.0f;
        style.ItemSpacing = ImVec2(6.0f, 4.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
    }
}
