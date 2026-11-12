#include <iostream>
#include <Finality.h>
#include <Editor/EditorApp.h>

int main(int argc, char const *argv[])
{
    FINALITY::RendererAPI api = FINALITY::RendererAPI::VULKAN;
    FINALITY::WindowSpec spec = { .title = "FINALITY Editor", .width = 1280, .height = 720, .isFullscreen = false };
    FINALITY::Application app;
    
    std::unique_ptr<FINALITY::EditorApp> game = std::make_unique<FINALITY::EditorApp>();
    auto directory = std::filesystem::absolute(argv[0]).parent_path();
    std::filesystem::path engineAssets;
    std::filesystem::path bundledAssets;
    const auto validAssets = [](const std::filesystem::path& path)
    {
        return std::filesystem::is_regular_file(path / "shaders/editor_grid.vert") &&
            std::filesystem::is_regular_file(path / "shaders/editor_grid.frag") &&
            std::filesystem::is_regular_file(path / "shaders/imported_model.frag");
    };
    for (auto candidate = directory; !candidate.empty(); candidate = candidate.parent_path())
    {
        if (validAssets(candidate / "FinalityEditor/assets")) { engineAssets = candidate / "FinalityEditor/assets"; break; }
        if (bundledAssets.empty() && validAssets(candidate / "assets")) bundledAssets = candidate / "assets";
        if (candidate == candidate.parent_path()) break;
    }
    if (engineAssets.empty()) engineAssets = bundledAssets;
    if (engineAssets.empty())
    {
        std::cerr << "Finality editor assets could not be found." << std::endl;
        return 1;
    }
    game->Configure(argc > 1 ? argv[1] : "", engineAssets);

    app.Initialize(api, spec, std::move(game));
    app.Update();
    app.Shutdown();

    return 0;
}
