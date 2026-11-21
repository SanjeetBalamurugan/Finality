/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for ImportProgress.h.
 */
#pragma once
#include <Assets/AssetRegistry.h>
#include <imgui.h>
#include <deque>
#include <functional>
#include <Core/Log.h>

namespace FINALITY
{
    /** @todo TODO: Document ImportProgress class semantics, ownership and usage. */
    class ImportProgress
    {
        std::deque<std::pair<std::string, std::function<void()>>> Jobs;
        int Frames = 0;
    public:
        bool IsBusy() const { return !Jobs.empty(); }
        void Clear() { Jobs.clear(); Frames = 0; }
        void Queue(std::string name, std::function<void()> work) { Jobs.emplace_back(std::move(name), std::move(work)); }
        void Import(const std::filesystem::path& source, const std::filesystem::path& destination)
        { Queue(source.filename().string(), [source,destination] { AssetRegistry::Import(source,destination); }); }
        void Update()
        {
            if (Jobs.empty()) { Frames = 0; return; }
            if (++Frames < 3) return;
            auto job = std::move(Jobs.front()); Jobs.pop_front(); Frames = 0;
            try { job.second(); } catch (const std::exception& error) { Logger::GetClientLogger()->error("Import failed: {}", error.what()); }
        }
        void Draw(bool compiling, const std::string& compilation)
        {
            const bool importing = !Jobs.empty();
            const char* title = importing ? "Importing Assets" : "Compiling Scripts";
            if (importing || compiling) ImGui::OpenPopup(title);
            for (const char* popup : {"Importing Assets", "Compiling Scripts"})
            {
                ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, {0.5f,0.5f});
                ImGui::SetNextWindowSize({480,120});
                if (ImGui::BeginPopupModal(popup, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
                {
                    const bool importPopup = std::string_view(popup) == "Importing Assets";
                    if ((importPopup && !importing) || (!importPopup && !compiling)) ImGui::CloseCurrentPopup();
                    else
                    {
                        ImGui::TextWrapped("%s", importPopup ? Jobs.front().first.c_str() : compilation.c_str());
                        ImGui::ProgressBar(-static_cast<float>(ImGui::GetTime()), {-1,0}, "Working...");
                    }
                    ImGui::EndPopup();
                }
            }
        }
    };
}
