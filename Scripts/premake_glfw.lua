project "GLFW"
	kind "StaticLib"
	language "C"
	staticruntime "off"
	warnings "off"

	files
	{
		"../vendor/glfw/include/GLFW/glfw3.h",
		"../vendor/glfw/include/GLFW/glfw3native.h",
		"../vendor/glfw/src/glfw_config.h",
		"../vendor/glfw/src/context.c",
		"../vendor/glfw/src/init.c",
		"../vendor/glfw/src/input.c",
		"../vendor/glfw/src/monitor.c",

		"../vendor/glfw/src/null_init.c",
		"../vendor/glfw/src/null_joystick.c",
		"../vendor/glfw/src/null_monitor.c",
		"../vendor/glfw/src/null_window.c",

		"../vendor/glfw/src/platform.c",
		"../vendor/glfw/src/vulkan.c",
		"../vendor/glfw/src/window.c",
	}

	filter "system:linux"
		pic "On"

		systemversion "latest"
		
		files
		{
			"../vendor/glfw/src/x11_init.c",
			"../vendor/glfw/src/x11_monitor.c",
			"../vendor/glfw/src/x11_window.c",
			"../vendor/glfw/src/xkb_unicode.c",
			"../vendor/glfw/src/posix_module.c",
			"../vendor/glfw/src/posix_time.c",
			"../vendor/glfw/src/posix_thread.c",
			"../vendor/glfw/src/posix_module.c",
			"../vendor/glfw/src/glx_context.c",
			"../vendor/glfw/src/egl_context.c",
			"../vendor/glfw/src/osmesa_context.c",
			"../vendor/glfw/src/linux_joystick.c"
		}

		defines
		{
			"_GLFW_X11"
		}

	filter "system:macosx"
		pic "On"

		files
		{
			"../vendor/glfw/src/cocoa_init.m",
			"../vendor/glfw/src/cocoa_monitor.m",
			"../vendor/glfw/src/cocoa_window.m",
			"../vendor/glfw/src/cocoa_joystick.m",
			"../vendor/glfw/src/cocoa_time.c",
			"../vendor/glfw/src/nsgl_context.m",
			"../vendor/glfw/src/posix_thread.c",
			"../vendor/glfw/src/posix_module.c",
			"../vendor/glfw/src/osmesa_context.c",
			"../vendor/glfw/src/egl_context.c"
		}

		defines
		{
			"_GLFW_COCOA"
		}

	filter "system:windows"
		systemversion "latest"

		files
		{
			"../vendor/glfw/src/win32_init.c",
			"../vendor/glfw/src/win32_joystick.c",
			"../vendor/glfw/src/win32_module.c",
			"../vendor/glfw/src/win32_monitor.c",
			"../vendor/glfw/src/win32_time.c",
			"../vendor/glfw/src/win32_thread.c",
			"../vendor/glfw/src/win32_window.c",
			"../vendor/glfw/src/wgl_context.c",
			"../vendor/glfw/src/egl_context.c",
			"../vendor/glfw/src/osmesa_context.c"
		}

		defines 
		{ 
			"_GLFW_WIN32",
			"_CRT_SECURE_NO_WARNINGS"
		}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter { "system:windows", "configurations:Debug-AS" }	
		runtime "Debug"
		symbols "on"
		sanitize { "Address" }
		runtimechecks "Off"
		incrementallink "Off"

	filter "configurations:Release"
		runtime "Release"
		optimize "speed"

    filter "configurations:Dist"
		runtime "Release"
		optimize "speed"
        symbols "off"