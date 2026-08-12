#pragma once
#include <Core/Log.h>
#include <Vulkan/vulkan.h>

#include <backends/imgui_impl_vulkan.h>

#include <stdio.h>

#define CHECK_VK_RESULT(res, msg) \
	if (res != VK_SUCCESS) { \
		FI_CORE_ERROR("On {}:{} - {}, {}", __FILE__, __LINE__, msg, static_cast<int>(res)); \
		std::exit(EXIT_FAILURE); \
	}