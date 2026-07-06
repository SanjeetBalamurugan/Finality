#include <iostream>
#include <Finality.h>

int main(int argc, char const *argv[])
{
    FINALITY::RendererAPI api = FINALITY::RendererAPI::VULKAN;
    FINALITY::Application app;
    app.Init(api);

    return 0;
}
