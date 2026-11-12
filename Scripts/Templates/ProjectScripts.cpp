#include <Scene/ScriptModule.h>
#include <Scene/Entity.h>
#include <Scene/Components.h>

class ProjectSpin : public FINALITY::ScriptableComponent
{
public:
    float Speed = 60.0f;
    glm::vec3 Axis{0,1,0};

private:
    void Update(float deltaTime) override
    {
        if (HasComponent<FINALITY::TransformComponent>()) GetComponent<FINALITY::TransformComponent>().Rotation += Axis * Speed * deltaTime;
    }
};

extern "C" __declspec(dllexport) uint64_t FinalityScriptABI()
{
    return FINALITY::ScriptModuleABI();
}

extern "C" __declspec(dllexport) bool FinalityRegisterScripts(FINALITY::ScriptRegistrationContext* context)
{
    return context && context->Add({"Project Spin",[]() { return std::make_unique<ProjectSpin>(); }});
}
