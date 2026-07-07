#pragma once

#define ENGINE_VERSION_MAJOR 1
#define ENGINE_VERSION_MINOR 0
#define ENGINE_VERSION_PATCH 0

#ifdef _WIN32
    #ifdef FINALITY_BUILD_DLL
        #define FAPI __declspec(dllexport)
    #else
        #define FAPI __declspec(dllimport)
    #endif
#else
    #ifdef FINALITY_BUILD_DLL
        #define FAPI __attribute__((visibility("default")))
    #else
        #define FAPI
    #endif
#endif

#ifdef FINALITY_INCLUDE_VULKAN
    #define GLFW_INCLUDE_VULKAN
#endif // FINALITY_INCLUDE_VULKAN
