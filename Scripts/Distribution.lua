project "Distribution"
    kind "Utility"
    dependson
    {
        "FinalityEditor",
        "FinalityPlayer",
        "FIBaker",
        "FIBuild",
        "FIScriptReflect",
        "FIShaderCompiler"
    }
    filter "configurations:Dist"
        postbuildcommands { '"%{cfg.targetdir}/FIBuild.exe" package-sdk --engine "' .. path.getabsolute("..") .. '" --bin "%{cfg.targetdir}" --output "%{cfg.targetdir}/SDK"' }
    filter {}
