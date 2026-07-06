#include <iostream>
#include <Finality.h>

int main(int argc, char const *argv[])
{
    FINALITY::RendererAPI api = FINALITY::RendererAPI::VULKAN;
    FINALITY::WindowSpec spec = { .title = "Hello", .width = 500, .height = 500, .isFullscreen = false };
    FINALITY::Application app;
    
    app.Initialize(api, spec);
    app.Update();
    app.Shutdown();

    return 0;
}
