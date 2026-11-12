#pragma once
#include <Assets/AssetRegistry.h>
#include <imgui.h>
#include <deque>
#include <functional>
#include <Core/Log.h>

namespace FINALITY
{
    class ImportProgress
    {
        inline static std::deque<std::pair<std::string, std::function<void()>>> Jobs;
        inline static int Frames = 0;
    public:
        static void Queue(std::string name, std::function<void()> work) { Jobs.emplace_back(std::move(name), std::move(work)); }
        static void Import(const std::filesystem::path& source, const std::filesystem::path& destination)
        { Queue(source.filename().string(), [source,destination] { AssetRegistry::Import(source,destination); }); }
        static void Update()
        {
            if (Jobs.empty()) { Frames = 0; return; }
            if (++Frames < 3) return;
            auto job = std::move(Jobs.front()); Jobs.pop_front(); Frames = 0;
            try { job.second(); } catch (const std::exception& error) { Logger::GetClientLogger()->error("Import failed: {}", error.what()); }
        }
        static void Draw(bool compiling, const std::string& compilation)
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
