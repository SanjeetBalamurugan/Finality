/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for BuildPanel.h.
 */
#pragma once
#include <Core/Process.h>
#include <Finality.h>
#include <future>
namespace FINALITY
{
/** @todo TODO: Document BuildPanel class semantics, ownership and usage. */
class BuildPanel
{
    /** @todo TODO: Document Entry struct semantics, ownership and usage. */
    struct Entry
    {
        std::string Path;
        bool Enabled = true;
    };
    std::filesystem::path Root, Output, Launch;
    std::vector<Entry> Scenes;
    std::string Company = "DefaultCompany", Product = "Finality Game", Version = "0.1.0", Status;
    int Width = 1280, Height = 720;
    bool Fullscreen = false, CopyPDB = false, Run = false;
    std::future<ProcessResult> Job;
    void Load(const Project &project);
    void Save();

  public:
    bool Visible = false, OpenPlayerSettings = false;
    bool IsBuilding() const { return Job.valid(); }
    void Draw(Project &project, const std::filesystem::path &engineAssets, const std::string &scene,
              bool canBuild);
    void PlayerProperties(const Project &project);
    void Start(Project &project, const std::filesystem::path &engineAssets, bool run);
};
}
