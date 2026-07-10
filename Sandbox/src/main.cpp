#include <iostream>
#include <Finality.h>
#include <scenes/SplashScreen.h>
#include <scenes/MainScene.h>

class SandboxGame : public FINALITY::Game
{
public:
    void Init() override
    {
        auto next = std::make_unique<SplashScreen>();
        FINALITY::SceneManager::GetInstance().SetScene(std::move(next));
        FINALITY::SceneManager::GetInstance().ChangeScene();
    };
    void Update(float ts) override {};
    void Destroy() override {};
};

int main(int argc, char const *argv[])
{
    FINALITY::RendererAPI api = FINALITY::RendererAPI::VULKAN;
    FINALITY::WindowSpec spec = { .title = "Hello", .width = 1000, .height = 600, .isFullscreen = false };
    FINALITY::Application app;
    
    std::unique_ptr<SandboxGame> game = std::make_unique<SandboxGame>();

    app.Initialize(api, spec, std::move(game));
    app.Update();
    app.Shutdown();

    return 0;
}
