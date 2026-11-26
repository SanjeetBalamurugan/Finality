newoption
{
    trigger = "engine-root",
    value = "PATH",
    description = "Finality engine checkout"
}
include "../NoVisualStudioFilters.lua"
newoption
{
    trigger = "engine-bin",
    value = "PATH",
    description = "Engine binaries matching the editor configuration"
}
newoption
{
    trigger = "project-root",
    value = "PATH",
    description = "Directory containing the Finality project"
}
newoption
{
    trigger = "project-name",
    value = "NAME",
    description = "Finality project name"
}
newoption
{
    trigger = "script-directory",
    value = "PATH",
    description = "Directory containing native script sources"
}
newoption
{
    trigger = "reflected-directory",
    value = "PATH",
    description = "Generated script translation units"
}
newoption
{
    trigger = "native-toolset",
    value = "NAME",
    description = "Engine compiler toolset"
}
newoption
{
    trigger = "native-version",
    value = "VERSION",
    description = "Engine MSVC version"
}
newoption
{
    trigger = "windows-sdk",
    value = "VERSION",
    description = "Engine Windows SDK"
}
newoption
{
    trigger = "native-configuration",
    value = "NAME",
    description = "Configuration matching the installed engine ABI"
}
local engine = assert(_OPTIONS["engine-root"])
local root = assert(_OPTIONS["project-root"])
local packagedSDK = os.isfile(engine .. "/FinalitySDK.json")
workspace (_OPTIONS["project-name"] or path.getname(root))
    startproject ((_OPTIONS["project-name"] or path.getname(root)) .. ".Scripts")
    architecture "x64"
    configurations { _OPTIONS["native-configuration"] or (packagedSDK and "Dist" or "Release") }
    location (root)
    toolset (_OPTIONS["native-toolset"])
    systemversion (_OPTIONS["windows-sdk"])
    filter "action:vs*"
        toolsversion (_OPTIONS["native-version"])
    filter {}
project ((_OPTIONS["project-name"] or path.getname(root)) .. ".Scripts")
    filename "GameScripts"
    targetname "GameScripts"
    location (root)
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"
    files
    {
        _OPTIONS["script-directory"] .. "/**.cpp",
        _OPTIONS["script-directory"] .. "/**.h",
        _OPTIONS["script-directory"] .. "/**.hpp",
        _OPTIONS["script-directory"] .. "/**.inl",
        _OPTIONS["script-directory"] .. "/**.vert",
        _OPTIONS["script-directory"] .. "/**.frag",
        _OPTIONS["script-directory"] .. "/**.glsl",
        _OPTIONS["script-directory"] .. "/**.comp"
    }
    filter { "files:**.cpp" }
        excludefrombuild "On"
    filter { "files:**/.Finality/NativeScripts/Reflected/**.cpp" }
        excludefrombuild "Off"
    filter {}
    targetdir (root .. "/.Finality/NativeScripts/bin/%{cfg.buildcfg}")
    objdir (root .. "/.Finality/NativeScripts/obj/%{cfg.buildcfg}")
    includedirs
    {
        _OPTIONS["script-directory"],
        engine .. "/FinalityEngine/src",
        engine .. "/vendor/glm",
        engine .. "/vendor/entt/include",
        engine .. "/vendor/spdlog/include",
        engine .. "/vendor/yaml-cpp/include",
        engine .. "/vendor/imgui"
    }
    libdirs { engine .. "/lib", _OPTIONS["engine-bin"] }
    links { "FinalityEngine" }
    defines
    {
        "GLM_FORCE_PURE",
        "GLM_FORCE_UNRESTRICTED_FLOAT",
        "IMGUI_API=__declspec(dllimport)"
    }
    buildoptions
    {
        "/utf-8",
        "/MP4",
        "/FS",
        "/Zc:__cplusplus"
    }
    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        defines { "DEBUG" }
    filter "configurations:Release"
        runtime "Release"
        optimize "On"
        defines { "NDEBUG" }
    filter "configurations:Dist"
        runtime "Release"
        optimize "Full"
        symbols "Off"
        linktimeoptimization "On"
        intrinsics "On"
        defines { "NDEBUG", "FINALITY_DIST" }
        linkoptions { "/OPT:REF", "/OPT:ICF" }
    filter {}
local engineProject = engine .. "/FinalityEngine/FinalityEngine.vcxproj"
if packagedSDK then
    project "Finality SDK"
        location (root .. "/.Finality/NativeScripts")
        kind "Utility"
        files
        {
            engine .. "/FinalitySDK.json",
            engine .. "/FinalityEngine/src/**.h",
            engine .. "/FinalityEngine/src/**.inl"
        }
elseif os.isfile(engineProject) then
    externalproject "FinalityEngine"
        location (engine .. "/FinalityEngine")
        filename "FinalityEngine"
        kind "SharedLib"
        language "C++"
        targetdir (_OPTIONS["engine-bin"])
        local handle = assert(io.open(engineProject, "r"))
        local contents = handle:read("*a")
        handle:close()
        uuid (assert(contents:match("<ProjectGuid>{(.-)}</ProjectGuid>"), "Engine project GUID is missing"))
else
    project "Finality Engine Sources"
        location (root .. "/.Finality/NativeScripts")
        kind "Utility"
        files { engine .. "/FinalityEngine/src/**.h", engine .. "/FinalityEngine/src/**.cpp" }
end
