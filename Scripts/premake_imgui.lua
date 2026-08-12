project "ImGui"
	kind "StaticLib"
	language "C++"
    staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	includedirs
	{
		"../vendor/imgui",
		"../vendor/glfw/include/"
	}

	files
	{
		"../vendor/imgui/imconfig.h",
		"../vendor/imgui/imgui.h",
		"../vendor/imgui/imgui.cpp",
		"../vendor/imgui/imgui_draw.cpp",
		"../vendor/imgui/imgui_internal.h",
		"../vendor/imgui/imgui_tables.cpp",
		"../vendor/imgui/imgui_widgets.cpp",
		"../vendor/imgui/imstb_rectpack.h",
		"../vendor/imgui/imstb_textedit.h",
		"../vendor/imgui/imstb_truetype.h",
		"../vendor/imgui/imgui_demo.cpp"
	}

    local vulkan = GetVulkanSDK()
    if vulkan then
        files {
            "../vendor/imgui/backends/imgui_impl_glfw.cpp",
            "../vendor/imgui/backends/imgui_impl_vulkan.cpp",
            "../vendor/imgui/backends/imgui_impl_glfw.h",
            "../vendor/imgui/backends/imgui_impl_vulkan.h"
        }

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
		cppdialect "C++17"

	filter "system:linux"
		pic "On"
		systemversion "latest"
		cppdialect "C++17"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

    filter "configurations:Dist"
		runtime "Release"
		optimize "on"
        symbols "off"