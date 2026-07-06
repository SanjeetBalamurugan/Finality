#pragma once

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
