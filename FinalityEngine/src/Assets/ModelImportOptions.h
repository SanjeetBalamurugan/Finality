#pragma once

namespace FINALITY
{
    struct ModelImportOptions
    {
        float Scale = 1.0f;
        bool GenerateNormals = true;
        bool FlipUVs = false;
        bool ImportMaterials = true;
        bool operator==(const ModelImportOptions&) const = default;
    };
}
