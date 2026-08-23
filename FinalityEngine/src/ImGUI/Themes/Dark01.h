#pragma once
#include <imgui.h>

namespace FINALITY
{
    static void ApplyDark01() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_WindowBg] = ImVec4(0.118f, 0.118f, 0.118f, 1.000f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.078f, 0.078f, 0.078f, 1.000f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.098f, 0.098f, 0.098f, 0.980f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.078f, 0.078f, 0.078f, 1.000f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.078f, 0.078f, 0.078f, 1.000f);
        colors[ImGuiCol_Header] = ImVec4(0.157f, 0.157f, 0.157f, 1.000f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.196f, 0.196f, 0.196f, 1.000f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.827f, 0.267f, 0.102f, 1.000f);

        colors[ImGuiCol_Button] = ImVec4(0.176f, 0.176f, 0.176f, 1.000f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.235f, 0.235f, 0.235f, 1.000f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.827f, 0.267f, 0.102f, 1.000f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.827f, 0.267f, 0.102f, 1.000f);

        colors[ImGuiCol_FrameBg] = ImVec4(0.059f, 0.059f, 0.059f, 1.000f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.137f, 0.137f, 0.137f, 1.000f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.235f, 0.235f, 0.235f, 1.000f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.827f, 0.267f, 0.102f, 1.000f);

        colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.176f, 0.176f, 0.176f, 1.000f);
        colors[ImGuiCol_TabActive] = ImVec4(0.118f, 0.118f, 0.118f, 1.000f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.118f, 0.118f, 0.118f, 1.000f);

        colors[ImGuiCol_Border] = ImVec4(0.039f, 0.039f, 0.039f, 1.000f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
        colors[ImGuiCol_Separator] = ImVec4(0.039f, 0.039f, 0.039f, 1.000f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.827f, 0.267f, 0.102f, 1.000f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.950f, 0.350f, 0.150f, 1.000f);

        colors[ImGuiCol_Text] = ImVec4(0.902f, 0.902f, 0.902f, 1.000f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.471f, 0.471f, 0.471f, 1.000f);

        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.WindowRounding = 0.0f;
        style.FrameRounding = 2.0f;
        style.GrabRounding = 2.0f;
        style.TabRounding = 2.0f;
        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.ItemSpacing = ImVec2(6.0f, 4.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    }
}
