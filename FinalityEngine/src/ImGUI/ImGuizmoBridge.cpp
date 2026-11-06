
#include "ImGuizmoBridge.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

bool FINALITY::ImGuizmoBridge::Manipulate(const glm::mat4& view,
    const glm::mat4& projection,
    int operation,
    glm::mat4& worldTransform)
{
    if (ImGui::GetCurrentContext() == nullptr)
        return false;

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const float width = max.x - min.x;
    const float height = max.y - min.y;

    if (width <= 0.0f || height <= 0.0f)
        return false;

    const ImGuizmo::OPERATION gizmoOperation =
        operation == 1 ? ImGuizmo::ROTATE :
        operation == 2 ? ImGuizmo::SCALE : ImGuizmo::TRANSLATE;

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetRect(min.x, min.y, width, height);

    glm::mat4 editedTransform = worldTransform;
    const bool changed = ImGuizmo::Manipulate(
        glm::value_ptr(view), glm::value_ptr(projection),
        gizmoOperation, ImGuizmo::WORLD,
        glm::value_ptr(editedTransform));

    if (!changed || !ImGuizmo::IsUsing())
        return false;

    worldTransform = editedTransform;
    return true;
}

void FINALITY::ImGuizmoBridge::DecomposeTransform(
    const glm::mat4& transform,
    glm::vec3& position,
    glm::vec3& rotationDegrees,
    glm::vec3& scale)
{
    ImGuizmo::DecomposeMatrixToComponents(
        glm::value_ptr(transform),
        glm::value_ptr(position),
        glm::value_ptr(rotationDegrees),
        glm::value_ptr(scale));
}
