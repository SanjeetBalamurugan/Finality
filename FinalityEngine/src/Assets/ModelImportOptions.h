/**
 * @file
 * @ingroup assets
 * @brief Defines model conversion and import policy.
 *
 * Import options control how source model data is converted into engine assets. Unit conversion and authored scale are import concerns; editing a placed entity scale is a separate scene operation.
 *
 * @par Usage
 * Use the model Import Inspector to persist changes and reimport. Inspect the converted bounds before compensating with entity transforms.
 */
#pragma once

namespace FINALITY
{
    /** @todo TODO: Document ModelImportOptions struct semantics, ownership and usage. */
    struct ModelImportOptions
    {
        float Scale = 1.0f;
        bool ConvertUnits = true;
        bool GenerateNormals = true;
        bool FlipUVs = false;
        bool ImportMaterials = true;
        bool operator==(const ModelImportOptions&) const = default;
    };
}
