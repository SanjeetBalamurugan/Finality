#include "RenderCommand.h"
#include <GLFW/glfw3.h>
#include "Application.h"

FINALITY::RenderDevice* FINALITY::RenderCommand::s_RenderDevice = nullptr;

void FINALITY::RenderCommand::SetWindowTitle(const std::string& title)
{
    FINALITY::Application::Get().GetWindow().SetWindowTitle(title);
}

void FINALITY::RenderCommand::SetCursorMode(bool hiddenAndLocked)
{
    FINALITY::Application::Get().GetWindow().SetCursorMode(hiddenAndLocked);
}