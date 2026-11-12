#pragma once
#include <Finality.h>
#include "EditorLayer.h"

namespace FINALITY
{
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
