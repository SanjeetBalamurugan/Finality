#pragma once
#include <Finality.h>
#include "EditorLayer.h"

namespace FINALITY
{
    class EditorApp : public FINALITY::Game
    {
    public:
        void Init() override;
        void Update(float ts) override;
        void Destroy() override;

    private:
        std::unique_ptr<EditorLayer> m_EditorLayer;
    };
}