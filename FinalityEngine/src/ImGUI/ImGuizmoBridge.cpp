
#include "ImGuizmoBridge.h"

#include <imgui.h>
#include "ImGuizmoApi.h"
#include <glm/gtc/type_ptr.hpp>
#include <Scene/Components.h>
#include <cmath>

void FINALITY::ImGuizmoBridge::BeginFrame()
{
    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
    ImGuizmo::BeginFrame();
}

bool FINALITY::ImGuizmoBridge::IsActive()
{
    return ImGuizmo::IsOver() || ImGuizmo::IsUsing();
}

bool FINALITY::ImGuizmoBridge::Manipulate(const glm::mat4& view,
    const glm::mat4& projection,
    int operation,
    glm::mat4& worldTransform, bool local)
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
        gizmoOperation, local ? ImGuizmo::LOCAL : ImGuizmo::WORLD,
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

void FINALITY::ImGuizmoBridge::SetLocalTransform(TransformComponent& component, const glm::mat4& local)
{
    component.Basis = glm::mat4(1);
    if (std::abs(glm::determinant(glm::mat3(local))) > 1.0e-8f)
    {
        DecomposeTransform(local,component.Position,component.Rotation,component.Scale);
        component.Basis = glm::inverse(component.GetTransformMatrix()) * local;
    }
    else
    {
        component.Position = glm::vec3(local[3]);
        component.Rotation = glm::vec3(0);
        component.Scale = glm::vec3(1);
        component.Basis = glm::translate(glm::mat4(1),-component.Position) * local;
    }
    component.Matrix = local;
}
