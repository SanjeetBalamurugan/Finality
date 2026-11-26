/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for ModelImportPanel.h.
 */
#pragma once
#include <Assets/ModelAsset.h>

namespace FINALITY
{
    class ExternalTools;

    /** @todo TODO: Document ModelImportPanel class semantics, ownership and usage. */
    class ModelImportPanel
    {
        std::string m_Reference, m_Message;
        ModelImportOptions m_Draft;

    public:
        void Draw(const std::string& reference, ExternalTools& tools);
    };
} // namespace FINALITY
