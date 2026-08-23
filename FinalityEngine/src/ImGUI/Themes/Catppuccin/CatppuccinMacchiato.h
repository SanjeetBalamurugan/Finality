#pragma once
#include <imgui.h>

namespace FINALITY
{
    static void ApplyCatppuccinMacchiato() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_WindowBg] = ImVec4(0.141f, 0.153f, 0.227f, 1.000f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.118f, 0.125f, 0.188f, 1.000f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.094f, 0.098f, 0.149f, 0.980f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.118f, 0.125f, 0.188f, 1.000f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.094f, 0.098f, 0.149f, 1.000f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.212f, 0.224f, 0.310f, 1.000f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.094f, 0.098f, 0.149f, 1.000f);
        colors[ImGuiCol_Header] = ImVec4(0.718f, 0.741f, 0.965f, 1.000f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.541f, 0.678f, 0.957f, 1.000f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.463f, 0.604f, 0.886f, 1.000f);

        colors[ImGuiCol_Button] = ImVec4(0.286f, 0.306f, 0.392f, 1.000f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.357f, 0.376f, 0.471f, 1.000f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.424f, 0.443f, 0.545f, 1.000f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.541f, 0.678f, 0.957f, 1.000f);

        colors[ImGuiCol_FrameBg] = ImVec4(0.212f, 0.224f, 0.310f, 1.000f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.286f, 0.306f, 0.392f, 1.000f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.357f, 0.376f, 0.471f, 1.000f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.718f, 0.741f, 0.965f, 1.000f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.541f, 0.678f, 0.957f, 1.000f);

        colors[ImGuiCol_Tab] = ImVec4(0.118f, 0.125f, 0.188f, 1.000f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.212f, 0.224f, 0.310f, 1.000f);
        colors[ImGuiCol_TabActive] = ImVec4(0.141f, 0.153f, 0.227f, 1.000f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.118f, 0.125f, 0.188f, 1.000f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.141f, 0.153f, 0.227f, 1.000f);

        colors[ImGuiCol_Border] = ImVec4(0.212f, 0.224f, 0.310f, 1.000f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
        colors[ImGuiCol_Separator] = ImVec4(0.212f, 0.224f, 0.310f, 1.000f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.718f, 0.741f, 0.965f, 1.000f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.541f, 0.678f, 0.957f, 1.000f);

        colors[ImGuiCol_Text] = ImVec4(0.792f, 0.827f, 0.961f, 1.000f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.647f, 0.678f, 0.796f, 1.000f);

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
