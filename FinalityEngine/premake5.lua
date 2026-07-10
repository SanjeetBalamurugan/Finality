project "FinalityEngine"
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"

    files {
        "src/**.h",
        "src/**.cpp",
        "src/**.c"
    }

    includedirs {
        "src",
        "../vendor/glfw/include/",
        "../vendor/spdlog/include/",
        "../vendor/glm",
        "../vendor/entt/include"
    }

    defines {
        "FINALITY_BUILD_DLL"
    }

    links {
        "GLFW"
    }

    dependson
    {
        "FIShaderCompiler"
    } -- Move this to editor project if i actually make it

    -- Importing Vulkan
    local vulkan = GetVulkanSDK()
    if vulkan then
        print("Vulkan SDK detected at: " .. vulkan.root)
        
        includedirs { vulkan.include }
        libdirs     { vulkan.lib }
        defines     { "FINALITY_INCLUDE_VULKAN" }

        filter "system:windows"
            links { "vulkan-1" }
        filter "system:linux or macosx"
            links { "vulkan" }
        filter {}
    else
        print("Vulkan SDK not found.")
    end

    filter "system:windows"
        systemversion "latest"

        defines {
            "_WINDLL"
        }
        defines { "GLFW_INCLUDE_NONE" }