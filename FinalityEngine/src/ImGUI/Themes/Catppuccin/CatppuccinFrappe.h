#pragma once
#include <imgui.h>

namespace FINALITY
{
    static void ApplyCatppuccinFrappe() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_WindowBg] = ImVec4(0.188f, 0.204f, 0.275f, 1.000f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.161f, 0.173f, 0.235f, 1.000f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.137f, 0.149f, 0.204f, 0.980f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.161f, 0.173f, 0.235f, 1.000f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.137f, 0.149f, 0.204f, 1.000f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.255f, 0.271f, 0.349f, 1.000f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.137f, 0.149f, 0.204f, 1.000f);
        colors[ImGuiCol_Header] = ImVec4(0.729f, 0.733f, 0.945f, 1.000f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.549f, 0.667f, 0.933f, 1.000f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.471f, 0.588f, 0.851f, 1.000f);

        colors[ImGuiCol_Button] = ImVec4(0.322f, 0.341f, 0.427f, 1.000f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.384f, 0.404f, 0.498f, 1.000f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.447f, 0.467f, 0.565f, 1.000f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.549f, 0.667f, 0.933f, 1.000f);

        colors[ImGuiCol_FrameBg] = ImVec4(0.255f, 0.271f, 0.349f, 1.000f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.322f, 0.341f, 0.427f, 1.000f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.384f, 0.404f, 0.498f, 1.000f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.729f, 0.733f, 0.945f, 1.000f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.549f, 0.667f, 0.933f, 1.000f);

        colors[ImGuiCol_Tab] = ImVec4(0.161f, 0.173f, 0.235f, 1.000f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.255f, 0.271f, 0.349f, 1.000f);
        colors[ImGuiCol_TabActive] = ImVec4(0.188f, 0.204f, 0.275f, 1.000f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.161f, 0.173f, 0.235f, 1.000f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.188f, 0.204f, 0.275f, 1.000f);

        colors[ImGuiCol_Border] = ImVec4(0.255f, 0.271f, 0.349f, 1.000f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
        colors[ImGuiCol_Separator] = ImVec4(0.255f, 0.271f, 0.349f, 1.000f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.729f, 0.733f, 0.945f, 1.000f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.549f, 0.667f, 0.933f, 1.000f);

        colors[ImGuiCol_Text] = ImVec4(0.776f, 0.816f, 0.961f, 1.000f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.647f, 0.678f, 0.808f, 1.000f);

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
