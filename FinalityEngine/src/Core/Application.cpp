#include "Application.h"
#include <iostream>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Log.h"

void FINALITY::Application::Init(RendererAPI api)
{
	Logger::Init();

	FI_CORE_INFO("FINALITY ENGINE");
}
