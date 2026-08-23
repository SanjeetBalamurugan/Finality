#pragma once
#include <imgui.h>

namespace FINALITY
{
    static void ApplyCatppuccinMocha() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_WindowBg] = ImVec4(0.118f, 0.118f, 0.180f, 1.000f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.094f, 0.094f, 0.145f, 1.000f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.067f, 0.067f, 0.106f, 0.980f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.094f, 0.094f, 0.145f, 1.000f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.067f, 0.067f, 0.106f, 1.000f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.192f, 0.196f, 0.267f, 1.000f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.067f, 0.067f, 0.106f, 1.000f);
        colors[ImGuiCol_Header] = ImVec4(0.796f, 0.651f, 0.969f, 1.000f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.537f, 0.706f, 0.980f, 1.000f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.455f, 0.624f, 0.898f, 1.000f);

        colors[ImGuiCol_Button] = ImVec4(0.271f, 0.278f, 0.353f, 1.000f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.345f, 0.357f, 0.439f, 1.000f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.424f, 0.439f, 0.525f, 1.000f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.537f, 0.706f, 0.980f, 1.000f);

        colors[ImGuiCol_FrameBg] = ImVec4(0.192f, 0.196f, 0.267f, 1.000f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.271f, 0.278f, 0.353f, 1.000f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.345f, 0.357f, 0.439f, 1.000f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.796f, 0.651f, 0.969f, 1.000f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.537f, 0.706f, 0.980f, 1.000f);

        colors[ImGuiCol_Tab] = ImVec4(0.094f, 0.094f, 0.145f, 1.000f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.192f, 0.196f, 0.267f, 1.000f);
        colors[ImGuiCol_TabActive] = ImVec4(0.118f, 0.118f, 0.180f, 1.000f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.094f, 0.094f, 0.145f, 1.000f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.118f, 0.118f, 0.180f, 1.000f);

        colors[ImGuiCol_Border] = ImVec4(0.192f, 0.196f, 0.267f, 1.000f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
        colors[ImGuiCol_Separator] = ImVec4(0.192f, 0.196f, 0.267f, 1.000f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.796f, 0.651f, 0.969f, 1.000f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.537f, 0.706f, 0.980f, 1.000f);

        colors[ImGuiCol_Text] = ImVec4(0.804f, 0.839f, 0.957f, 1.000f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.651f, 0.678f, 0.784f, 1.000f);

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
