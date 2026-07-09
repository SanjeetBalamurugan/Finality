project "FIShaderCompiler"
    kind "ConsoleApp"
    language "C++"

    files {
        "src/**.h",
        "src/**.cpp"
    }

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