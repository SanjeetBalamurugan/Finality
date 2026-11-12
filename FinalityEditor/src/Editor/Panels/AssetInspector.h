#pragma once

#include <Editor/AssetPicker.h>

namespace FINALITY
{
    class AssetInspector
    {
    public:
        void Draw(uint64_t selected, AssetPicker& picker, bool* open = nullptr);
        void Clear();

    private:
        uint64_t m_Selected = 0;
        int64_t m_Modified = 0;
        MaterialComponent m_Material;
        std::string m_Source;
        size_t m_EntityCount = 0;
    };
}
