/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for EntityInspector.h.
 */
#pragma once

#include <Editor/AssetPicker.h>

namespace FINALITY
{
    class ExternalTools;
    void DrawEntityInspector(Scene* scene, entt::entity selected, AssetPicker& picker, ExternalTools& tools, bool* open = nullptr);
    void DrawMaterialProperties(MaterialComponent& component, AssetPicker& picker, std::function<MaterialComponent*()> target);
    void ClearInspectorClipboard();
}
