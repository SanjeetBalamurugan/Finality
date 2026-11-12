#include "AssetPreview.h"
#include <Assets/AssetManager.h>
#include <Vulkan/VKTexture.h>
#include <Renderer/Renderer.h>
#include <unordered_map>
#include <Assets/ModelAsset.h>
#include <Assets/MaterialAsset.h>
#include <Utils/Serializers/MaterialSerializer.h>
#include <Core/RenderTarget.h>
#include <chrono>
#include <algorithm>
#include <cmath>

namespace
{
    struct Preview
    {
        std::shared_ptr<FINALITY::Texture> Texture;
        VkDescriptorSet Descriptor = VK_NULL_HANDLE;
    };
    std::unordered_map<std::string, Preview> s_Previews;
    std::vector<Preview> s_RetiredPreviews;
    struct RenderedPreview
    {
        std::shared_ptr<FINALITY::RenderTarget> Target;
        void* Handle = nullptr;
        FINALITY::ProjectAssetType Type = FINALITY::ProjectAssetType::Unknown;
        std::string Signature;
        uint64_t Used = 0;
        bool Pending = true;
        std::chrono::steady_clock::time_point NextCheck{};
    };
    std::unordered_map<std::string, RenderedPreview> s_Rendered;
    std::vector<std::shared_ptr<FINALITY::RenderTarget>> s_RetiredTargets;
    int s_Resolution = 128, s_Capacity = 128;
    uint64_t s_Tick = 0;

    std::string Stamp(const std::string& reference)
    {
        const auto path = FINALITY::AssetRegistry::Resolve(reference);
        std::error_code error;
        const auto time = std::filesystem::last_write_time(path,error);
        return error ? "missing" : std::to_string(time.time_since_epoch().count());
    }

    std::string Fingerprint(const std::string& reference, FINALITY::ProjectAssetType type)
    {
        std::string value = Stamp(reference);
        auto material = [&](const std::string& asset)
        {
            value += Stamp(asset);
            FINALITY::MaterialDefinition definition;
            if (FINALITY::MaterialSerializer::Deserialize(FINALITY::AssetRegistry::Resolve(asset),definition))
                value += Stamp(definition.VertexShader) + Stamp(definition.FragmentShader) + Stamp(definition.TexturePath);
        };
        if (type == FINALITY::ProjectAssetType::Material) material(reference);
        else if (type == FINALITY::ProjectAssetType::Model)
            for (const auto& definition : FINALITY::ModelAsset::Load(reference)->Materials) if (!definition.Asset.empty()) material(definition.Asset);
        return value;
    }
}

void FINALITY::AssetPreview::Configure(int resolution, int capacity)
{
    resolution = std::clamp(resolution,64,512); capacity = std::clamp(capacity,16,512);
    if (resolution != s_Resolution || capacity != s_Capacity) Clear();
    s_Resolution = resolution; s_Capacity = capacity;
}

void* FINALITY::AssetPreview::RenderedHandle(const std::string& reference, ProjectAssetType type)
{
    if (type != ProjectAssetType::Model && type != ProjectAssetType::Material) return nullptr;
    auto found = s_Rendered.find(reference);
    if (found == s_Rendered.end())
    {
        if (s_Rendered.size() >= static_cast<size_t>(s_Capacity))
        {
            const auto oldest = std::min_element(s_Rendered.begin(),s_Rendered.end(),[](const auto& a, const auto& b) { return a.second.Used < b.second.Used; });
            if (oldest->second.Target) s_RetiredTargets.push_back(std::move(oldest->second.Target));
            s_Rendered.erase(oldest);
        }
        found = s_Rendered.emplace(reference,RenderedPreview{}).first;
        found->second.Type = type;
    }
    auto& preview = found->second;
    preview.Used = ++s_Tick;
    const auto now = std::chrono::steady_clock::now();
    if (now >= preview.NextCheck)
    {
        preview.NextCheck = now + std::chrono::seconds(1);
        try
        {
            const auto signature = Fingerprint(reference,type);
            if (signature != preview.Signature) { preview.Signature = signature; preview.Pending = true; }
        }
        catch (const std::exception&) { return preview.Handle; }
    }
    return preview.Handle;
}

void FINALITY::AssetPreview::Update()
{
    auto* device = Renderer::GetDevice();
    if (!device) return;
    int generated = 0;
    for (auto& [reference, preview] : s_Rendered)
    {
        if (!preview.Pending || generated++ >= 1) continue;
        preview.Pending = false;
        std::shared_ptr<RenderTarget> target;
        try
        {
            std::vector<RenderPacket> queue;
            glm::vec3 center(0);
            float radius = 0.65f;
            if (preview.Type == ProjectAssetType::Model)
            {
                const auto model = ModelAsset::Load(reference);
                center = (model->Minimum + model->Maximum) * 0.5f;
                radius = std::max(glm::length(model->Maximum - model->Minimum) * 0.5f,0.01f);
                queue = ModelAsset::BuildPreview(reference);
            }
            else
            {
                MaterialDefinition definition;
                const auto path = AssetRegistry::Resolve(reference);
                if (MaterialSerializer::Deserialize(path,definition) && !definition.TexturePath.empty()) AssetManager::InvalidateAssetPath(AssetRegistry::Resolve(definition.TexturePath));
                const auto asset = MaterialAsset::Create(path,device);
                if (!asset) throw std::runtime_error("Material preview could not load the material");
                const auto material = asset->CreateInstance();
                if (!material) throw std::runtime_error("Material preview properties are invalid");
                RenderPacket packet;
                packet.MeshData = Mesh::Create("builtin://sphere"); packet.PipelineInstance = material->GetPipeline();
                packet.Textures = material->GetTextures(); packet.CustomPushData = material->GetRawDataBuffer(); packet.MaterialKey = asset.get();
                queue.push_back(std::move(packet));
            }
            if (queue.empty()) throw std::runtime_error("No renderable model geometry");
            target = device->CreateRenderTarget(s_Resolution,s_Resolution);
            target->ClearColor = {0.075f,0.09f,0.12f,1};
            Camera camera(38,1,std::max(radius * 0.001f,0.0001f),std::max(radius * 20,10.0f));
            const auto position = center + glm::normalize(glm::vec3(1,0.55f,1.5f)) * radius * 3.3f;
            camera.SetPosition(position);
            const auto direction = glm::normalize(center - position);
            camera.SetRotation({glm::degrees(std::asin(direction.y)),glm::degrees(std::atan2(direction.x,-direction.z)),0});
            device->UpdateRenderTargetCamera(target,&camera);
            device->RenderSceneToTarget(target,queue);
            target->SourceCamera = nullptr;
            const auto handle = device->GetRenderTargetTextureHandle(target);
            if (preview.Target) s_RetiredTargets.push_back(std::move(preview.Target));
            preview.Target = std::move(target); preview.Handle = handle;
        }
        catch (const std::exception& error)
        {
            if (target) s_RetiredTargets.push_back(std::move(target));
            FI_CORE_WARN("Preview '{}': {}",reference,error.what());
        }
    }
}

void* FINALITY::AssetPreview::TextureHandle(const std::string& reference)
{
    if (const auto found = s_Previews.find(reference); found != s_Previews.end()) return found->second.Descriptor;
    if (s_Previews.size() >= 256) return nullptr;
    auto texture = AssetManager::CreateAsset<Texture>(reference);
    if (!texture) return nullptr;
    auto* vulkan = dynamic_cast<VKTexture*>(texture.get());
    if (!vulkan) return nullptr;
    Preview preview;
    preview.Texture = std::move(texture);
    preview.Descriptor = ImGui_ImplVulkan_AddTexture(vulkan->GetSampler(), vulkan->GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    const auto descriptor = preview.Descriptor;
    s_Previews.emplace(reference, std::move(preview));
    return descriptor;
}

void FINALITY::AssetPreview::Clear()
{
    for (auto& [path, preview] : s_Rendered) if (preview.Target) s_RetiredTargets.push_back(std::move(preview.Target));
    s_Rendered.clear();
    for (auto& [path, preview] : s_Previews) s_RetiredPreviews.push_back(std::move(preview));
    s_Previews.clear();
}

void FINALITY::AssetPreview::Flush()
{
    if (s_RetiredPreviews.empty() && s_RetiredTargets.empty()) return;
    if (Renderer::GetDevice()) Renderer::GetDevice()->WaitIdle();
    for (auto& preview : s_RetiredPreviews)
        if (preview.Descriptor) ImGui_ImplVulkan_RemoveTexture(preview.Descriptor);
    s_RetiredPreviews.clear();
    for (auto& target : s_RetiredTargets) Renderer::GetDevice()->DestroyRenderTargetGpuResources(*target);
    s_RetiredTargets.clear();
}
