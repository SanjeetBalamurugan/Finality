/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for EditorApp.h.
 */
#pragma once
#include <Finality.h>
#include "EditorLayer.h"

namespace FINALITY
{
    /** @todo TODO: Document EditorApp class semantics, ownership and usage. */
    class EditorApp : public FINALITY::Game
    {
    public:
        void Configure(const std::string& initialProject, const std::filesystem::path& engineAssets)
        { m_InitialProject = initialProject; m_EngineAssets = engineAssets; }
        void Init() override;
        void Update(float ts) override;
        void Destroy() override;

    private:
        std::string m_InitialProject;
        std::filesystem::path m_EngineAssets;
        std::unique_ptr<EditorLayer> m_EditorLayer;
    };
}
