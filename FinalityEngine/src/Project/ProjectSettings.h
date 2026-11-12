#pragma once

#include <Assets/ModelImportOptions.h>
#include <glm/glm.hpp>
#include <string>

namespace FINALITY
{
    struct ProjectSettings
    {
        float CameraSpeed = 4.0f;
        float CameraSprint = 3.0f;
        float CameraSensitivity = 0.12f;
        glm::vec4 SceneBackground{0.075f,0.09f,0.11f,1};
        bool Grid = true;
        float GridSpacing = 1.0f;
        float GridFadeDistance = 180.0f;
        int ThumbnailSize = 128;
        int ThumbnailCapacity = 128;
        bool AutoReimport = true;
        float AssetScanInterval = 2.0f;
        bool AutoBuildScripts = true;
        float ScriptBuildDelay = 0.75f;
        std::string ScriptDirectory = "Assets/Scripts";
        std::string Theme = "Dark01";
        ModelImportOptions Models;
    };
}
