#pragma once
#include <imgui.h>

namespace FINALITY
{
    static void ApplyLight01()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // --- Window & Background Panels ---
        colors[ImGuiCol_WindowBg] = ImVec4(0.220f, 0.220f, 0.220f, 1.000f); // (#383838)
        colors[ImGuiCol_ChildBg] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f); // (#333333)
        colors[ImGuiCol_PopupBg] = ImVec4(0.180f, 0.180f, 0.180f, 0.960f); // Dropdowns and context menus
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f); // Main top menu bar

        // --- Headers & Title Bars ---
        colors[ImGuiCol_TitleBg] = ImVec4(0.149f, 0.149f, 0.149f, 1.000f); // Title bar flat grey
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.149f, 0.149f, 0.149f, 1.000f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.149f, 0.149f, 0.149f, 1.000f);
        colors[ImGuiCol_Header] = ImVec4(0.173f, 0.349f, 0.545f, 1.000f); // selection/focus blue (#2C598B)
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.231f, 0.424f, 0.639f, 1.000f); // Lighter blue selection hover
        colors[ImGuiCol_HeaderActive] = ImVec4(0.157f, 0.314f, 0.502f, 1.000f);

        // --- Interactive Elements (Buttons, Checkboxes) ---
        colors[ImGuiCol_Button] = ImVec4(0.302f, 0.302f, 0.302f, 1.000f); // slate grey (#4D4D4D)
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.345f, 0.345f, 0.345f, 1.000f); // Slightly lighter hover (#585858)
        colors[ImGuiCol_ButtonActive] = ImVec4(0.255f, 0.255f, 0.255f, 1.000f); // Darker clicked slate (#414141)
        colors[ImGuiCol_CheckMark] = ImVec4(0.769f, 0.769f, 0.769f, 1.000f); // Light grey check mark

        // --- Input Fields (Text Boxes, Sliders) ---
        colors[ImGuiCol_FrameBg] = ImVec4(0.165f, 0.165f, 0.165f, 1.000f); // Sunken fields input dark grey (#2A2A2A)
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.184f, 0.184f, 0.184f, 1.000f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.400f, 0.400f, 0.400f, 1.000f); // Inline slider tick line
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.173f, 0.349f, 0.545f, 1.000f); // Highlights blue when clicked

        // --- Tabs (For Editor Docking layout) ---
        colors[ImGuiCol_Tab] = ImVec4(0.220f, 0.220f, 0.220f, 1.000f); // Inactive tab matches window background
        colors[ImGuiCol_TabHovered] = ImVec4(0.267f, 0.267f, 0.267f, 1.000f);
        colors[ImGuiCol_TabActive] = ImVec4(0.220f, 0.220f, 0.220f, 1.000f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.220f, 0.220f, 0.220f, 1.000f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.220f, 0.220f, 0.220f, 1.000f);

        // --- Borders & Separators ---
        colors[ImGuiCol_Border] = ImVec4(0.133f, 0.133f, 0.133f, 1.000f); // Dark separation borders (#222222)
        colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
        colors[ImGuiCol_Separator] = ImVec4(0.133f, 0.133f, 0.133f, 1.000f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.173f, 0.349f, 0.545f, 1.000f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.231f, 0.424f, 0.639f, 1.000f);

        // --- Text Colors ---
        colors[ImGuiCol_Text] = ImVec4(0.800f, 0.800f, 0.800f, 1.000f); // Muted crisp silver text (#CC0CC0 -> #CDCDCD equivalent)
        colors[ImGuiCol_TextDisabled] = ImVec4(0.502f, 0.502f, 0.502f, 1.000f); // Ghosted hierarchy elements
        style.WindowPadding = ImVec2(6.0f, 6.0f);                     // Tight Unity padding layout

        // --- Layout Properties (Flat Look) ---
        style.WindowRounding = 0.0f;
        style.FrameRounding = 3.0f;  // Small subtle roundness for input boxes/buttons
        style.GrabRounding = 1.0f;
        style.TabRounding = 0.0f;  // Tabs are flat cuts
        style.WindowBorderSize = 1.0f;  // Clear, crisp panel borders
        style.FrameBorderSize = 1.0f;  // Outlined properties
    }
}