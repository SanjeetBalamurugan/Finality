/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for AssetInspector.h.
 */
#pragma once

#include <Editor/AssetPicker.h>
#include "ModelImportPanel.h"

namespace FINALITY
{
    class ExternalTools;
    /** @todo TODO: Document AssetInspector class semantics, ownership and usage. */
    class AssetInspector
    {
    public:
        void Draw(uint64_t selected, AssetPicker& picker, ExternalTools& tools, bool* open = nullptr);
        void Clear();

    private:
        uint64_t m_Selected = 0;
        int64_t m_Modified = 0;
        MaterialComponent m_Material;
        std::string m_Source;
        size_t m_EntityCount = 0;
        ModelImportPanel m_ModelImport;
        std::string m_TextureImportError;
    };
}
