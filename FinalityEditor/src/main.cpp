#include <iostream>
#include <Finality.h>
#include <Editor/EditorApp.h>

int main(int argc, char const *argv[])
{
    FINALITY::RendererAPI api = FINALITY::RendererAPI::VULKAN;
    FINALITY::WindowSpec spec = { .title = "FINALITY Editor", .width = 1280, .height = 720, .isFullscreen = false };
    FINALITY::Application app;
    
    std::unique_ptr<FINALITY::EditorApp> game = std::make_unique<FINALITY::EditorApp>();

    app.Initialize(api, spec, std::move(game));
    app.Update();
    app.Shutdown();

    return 0;
}
