function GetVulkanSDK()
    local sdkPath = os.getenv("VULKAN_SDK")
    
    if not sdkPath then
        return nil
    end

    local includeDir = sdkPath .. "/Include"
    local libDir     = sdkPath .. "/Lib"

    if os.target() == "macosx" then
        includeDir = sdkPath .. "/macOS/include"
        libDir     = sdkPath .. "/macOS/lib"
    elseif os.target() == "linux" then
        includeDir = sdkPath .. "/include"
        libDir     = sdkPath .. "/lib"
    end

    return {
        root    = sdkPath,
        include = includeDir,
        lib     = libDir
    }
end