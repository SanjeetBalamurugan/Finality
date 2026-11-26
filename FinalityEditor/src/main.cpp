#include <iostream>
#include <Finality.h>
#include <Editor/EditorApp.h>
#include <Editor/EditorDependencies.h>

int main(int argc, char const* argv[])
{
    const auto directory = FINALITY::Process::ExecutablePath().parent_path();
    auto dependencies = FINALITY::EditorDependencies::Inspect(directory);
    if (argc > 1 && std::string(argv[1]) == "--check-dependencies")
    {
        std::cout << dependencies.Describe();
        return dependencies.Errors.empty() ? 0 : 1;
    }
    if (!FINALITY::EditorDependencies::ConfirmStartup(directory, dependencies))
    {
        return 1;
    }

    try
    {
        FINALITY::RendererAPI api = FINALITY::RendererAPI::VULKAN;
        FINALITY::WindowSpec spec = { .title = "FINALITY Editor", .width = 1280, .height = 720, .isFullscreen = false };
        FINALITY::Application app;

        std::unique_ptr<FINALITY::EditorApp> game = std::make_unique<FINALITY::EditorApp>();
        game->Configure(argc > 1 ? argv[1] : "", dependencies.Assets);

        app.Initialize(api, spec, std::move(game));
        app.Update();
        app.Shutdown();
    }
    catch (const std::exception& error)
    {
        FINALITY::EditorDependencies::ShowStartupError(error.what());
        return 1;
    }

    return 0;
}
