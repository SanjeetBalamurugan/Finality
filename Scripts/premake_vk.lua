function GetVulkanSDK()
    if _ACTION == "clean" then
        return nil
    end
    local sdkPath = os.getenv("VULKAN_SDK")

    if not sdkPath or sdkPath == "" then
        error("Vulkan SDK is required to build Finality from source. Install the LunarG Vulkan SDK, restart Visual Studio/your terminal, and regenerate the solution. VULKAN_SDK is not set.")
    end

    local includeDir = sdkPath .. "/Include"
    local libDir = sdkPath .. "/Lib"

    if os.target() == "macosx" then
        includeDir = sdkPath .. "/macOS/include"
        libDir = sdkPath .. "/macOS/lib"
    elseif os.target() == "linux" then
        includeDir = sdkPath .. "/include"
        libDir = sdkPath .. "/lib"
    end

    if not os.isfile(path.join(includeDir, "vulkan/vulkan.h")) then
        error("VULKAN_SDK points to an invalid SDK: " .. sdkPath .. ". Install the Vulkan SDK and regenerate the solution.")
    end
    if os.target() == "windows" and not os.isfile(path.join(sdkPath, "Bin/glslc.exe")) then
        error("The Vulkan SDK shader compiler is missing: " .. path.join(sdkPath, "Bin/glslc.exe"))
    end

    return {
        root = sdkPath,
        include = includeDir,
        lib = libDir
    }
end
