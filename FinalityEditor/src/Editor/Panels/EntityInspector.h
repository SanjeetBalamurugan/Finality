#pragma once

#include <Editor/AssetPicker.h>

namespace FINALITY
{
    void DrawEntityInspector(Scene* scene, entt::entity selected, AssetPicker& picker, bool* open = nullptr);
    void DrawMaterialProperties(MaterialComponent& component, AssetPicker& picker, std::function<MaterialComponent*()> target);
    void ClearInspectorClipboard();
}
