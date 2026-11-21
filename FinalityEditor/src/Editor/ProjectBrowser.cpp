#include "EditorLayer.h"
#include <ImGUI/AssetPreview.h>
#include <algorithm>
#include <fstream>
#include "ExternalTools.h"
#include "Prefab.h"
#include "ImportProgress.h"
#include <imgui_internal.h>

void FINALITY::EditorLayer::DrawContentBrowser(entt::entity selectedEntity)
{
    namespace fs = std::filesystem;
    if (!ImGui::Begin("Content Browser", &m_ShowContentBrowser))
    {
        ImGui::End();
        return;
    }
    static int typeFilter = 0;
    m_ContentBrowserVisible = true;
    m_ContentBrowserFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    auto browserMenu = [&]()
    {
        if (ImGui::BeginMenu("Create"))
        {
            if (ImGui::MenuItem("Folder"))
            {
                auto path = fs::path(m_ContentDirectory) / "New Folder";
                for (int index = 1; fs::exists(path); ++index) path = fs::path(m_ContentDirectory) / ("New Folder " + std::to_string(index));
                fs::create_directories(path);
            }
            if (ImGui::MenuItem("Native Script"))
            {
                int index = 1;
                while (fs::exists(fs::path(m_ContentDirectory) / ("NewScript" + std::to_string(index) + ".cpp"))) ++index;
                const auto name = "NewScript" + std::to_string(index);
                std::ofstream(fs::path(m_ContentDirectory) / (name + ".cpp")) << "#include <Scene/ScriptModule.h>\nclass " << name << " : public FINALITY::ScriptableComponent {};\n";
                AssetRegistry::Scan();
            }
            if (ImGui::MenuItem("Material"))
            {
                auto path = fs::path(m_ContentDirectory) / "New Material.fmat";
                for (int index = 1; fs::exists(path); ++index) path = fs::path(m_ContentDirectory) / ("New Material " + std::to_string(index) + ".fmat");
                const auto source = m_Session.ProjectData.GetAssetsDirectory() / "materials/Default.fmat";
                if (fs::exists(source)) fs::copy_file(source,path);
                AssetRegistry::Scan();
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Show in Explorer")) ShellExecuteW(nullptr,L"open",fs::path(m_ContentDirectory).c_str(),nullptr,nullptr,SW_SHOWNORMAL);
        if (ImGui::MenuItem("Copy Path")) ImGui::SetClipboardText(m_ContentDirectory.c_str());
        if (ImGui::MenuItem("Import New Asset..."))
        {
            const auto path = FileDialogs::OpenFile("All Files\0*.*\0\0");
            if (!path.empty()) m_Session.Imports.Import(path,fs::path(m_ContentDirectory).lexically_relative(m_Session.ProjectData.GetDirectory()));
        }
        if (ImGui::MenuItem("Refresh")) m_Session.Imports.Queue("Refreshing asset database", [] { AssetRegistry::Scan(); });
    };
    if (ImGui::BeginPopupContextWindow("Browser Actions", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        browserMenu();
        if (ImGui::MenuItem("Project Settings")) m_ShowProjectSettings = true;
        ImGui::EndPopup();
    }
    const char *types[] = {"All Types", "Textures", "Models", "Materials", "Shaders", "Scenes", "Scripts", "Prefabs"};
    if (ImGui::Button("Import"))
        ImGui::OpenPopup("Import Assets");
    if (ImGui::BeginPopup("Import Assets"))
    {
        if (ImGui::MenuItem("Asset..."))
        {
            const auto path = FileDialogs::OpenFile(
                "Assets\0*.obj;*.gltf;*.glb;*.fbx;*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.fmat;*.fscene;*."
                "vert;*.frag;*.cpp;*.h\0Models\0*.obj;*.gltf;*.glb;*.fbx\0All Files\0*.*\0\0");
            if (!path.empty())
            {
                AssetPreview::Clear();
                m_Session.Imports.Import(path, fs::path(m_ContentDirectory).lexically_relative(m_Session.ProjectData.GetDirectory()));
            }
        }
        if (ImGui::MenuItem("Folder..."))
        {
            const auto path = FileDialogs::OpenFolder();
            if (!path.empty())
            {
                AssetPreview::Clear();
                m_Session.Imports.Import(path, fs::path(m_ContentDirectory).lexically_relative(m_Session.ProjectData.GetDirectory()));
            }
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh"))
    {
        AssetPreview::Clear();
        AssetRegistry::Scan();
    }
    ImGui::SameLine();
    if (ImGui::Button("New Material"))
    {
        const auto destination = fs::path(m_ContentDirectory) / "New Material.fmat";
        const auto source = m_Session.ProjectData.GetAssetsDirectory() / "materials/Default.fmat";
        if (!fs::exists(destination) && fs::is_regular_file(source))
            fs::copy_file(source, destination);
        AssetRegistry::Scan();
    }
    ImGui::SameLine();
    if (ImGui::Button(m_ContentGrid ? "Grid" : "List"))
        m_ContentGrid = !m_ContentGrid;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::SliderFloat("##Thumbnail Size", &m_ContentThumbnailSize, 44, 112, "%.0f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Thumbnail size");
    ImGui::SetNextItemWidth(std::max(100.0f, ImGui::GetContentRegionAvail().x - 140));
    if (m_FocusSearch == "Content Browser") { ImGui::SetKeyboardFocusHere(); m_FocusSearch.clear(); }
    EditText("##Search Assets", m_AssetSearch);
    if (!ImGui::IsItemActive() && m_AssetSearch.empty())
    {
        const auto origin = ImGui::GetItemRectMin();
        const auto padding = ImGui::GetStyle().FramePadding;
        ImGui::GetWindowDrawList()->AddText({origin.x + padding.x, origin.y + padding.y},
                                            ImGui::GetColorU32(ImGuiCol_TextDisabled), "Search assets");
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##Type", &typeFilter, types, 8);
    const auto root = m_Session.ProjectData.GetAssetsDirectory();
    fs::path directory(m_ContentDirectory);
    if (!fs::is_directory(directory))
    {
        directory = root;
        m_ContentDirectory = root.string();
    }
    if (ImGui::Button("Assets"))
        m_ContentDirectory = root.string();
    auto accumulated = root;
    for (const auto &part : directory.lexically_relative(root))
    {
        if (part == ".")
            continue;
        accumulated /= part;
        ImGui::SameLine();
        if (ImGui::Button(part.string().c_str()))
            m_ContentDirectory = accumulated.string();
    }
    ImGui::Separator();
    std::error_code error;
    std::string openScene;
    uint64_t moveID = 0;
    uint64_t duplicateID = 0;
    uint64_t reimportID = 0;
    uint64_t instantiateID = 0;
    static uint64_t renameID = 0;
    static std::string renamePath;
    static uint64_t deleteID = 0;
    if (m_Session.State == EditorState::Edit && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        !ImGui::GetIO().WantTextInput && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
    {
        if (const auto* asset = AssetRegistry::Find(m_Session.SelectedAsset))
        {
            if (m_Session.Preferences.Pressed(EditorAction::Delete)) deleteID = asset->ID;
            if (m_Session.Preferences.Pressed(EditorAction::Duplicate)) duplicateID = asset->ID;
            if (m_Session.Preferences.Pressed(EditorAction::Rename))
            { renameID = moveID = asset->ID; renamePath = asset->Path; }
            if (m_Session.Preferences.Pressed(EditorAction::OpenAsset))
            {
                const auto path = m_Session.ProjectData.GetDirectory()/asset->Path;
                if (asset->Type == ProjectAssetType::Scene) openScene = path.string();
                else if (asset->Type == ProjectAssetType::Model || asset->Type == ProjectAssetType::Prefab) instantiateID = asset->ID;
                else if (asset->Type == ProjectAssetType::Script) m_Session.Tools.OpenScript(path);
            }
        }
        if (m_Session.Preferences.Pressed(EditorAction::ParentFolder) && directory != root) m_ContentDirectory = directory.parent_path().string();
    }
    if (ImGui::BeginTable("Content Layout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 150);
        ImGui::TableSetupColumn("Contents", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        if (ImGui::BeginChild("Folders"))
        {
            auto folder = [&](auto &&self, const fs::path &path, int depth) -> void {
                if (depth > 24)
                    return;
                std::vector<fs::path> children;
                std::error_code folderError;
                for (fs::directory_iterator it(path, folderError), end; it != end && !folderError;
                     it.increment(folderError))
                    if (it->is_directory() && !it->is_symlink())
                        children.push_back(it->path());
                std::sort(children.begin(), children.end());
                ImGui::PushID(path.string().c_str());
                auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (directory == path)
                    flags |= ImGuiTreeNodeFlags_Selected;
                if (children.empty())
                    flags |= ImGuiTreeNodeFlags_Leaf;
                if (path == root)
                    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                const bool open = ImGui::TreeNodeEx(path == root ? "Assets" : path.filename().string().c_str(), flags);
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                    m_ContentDirectory = path.string();
                if (ImGui::BeginDragDropTarget())
                {
                    if (const auto* payload = ImGui::AcceptDragDropPayload("FinalityAsset"); payload && payload->DataSize == sizeof(uint64_t))
                    {
                        const auto id = *static_cast<const uint64_t*>(payload->Data);
                        if (const auto* asset = AssetRegistry::Find(id))
                            AssetRegistry::Move(id, (path / fs::path(asset->Path).filename()).lexically_relative(m_Session.ProjectData.GetDirectory()));
                    }
                    ImGui::EndDragDropTarget();
                }
                if (open)
                {
                    for (const auto &child : children)
                        self(self, child, depth + 1);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            };
            folder(folder, root, 0);
        }
        ImGui::EndChild();
        ImGui::TableSetColumnIndex(1);
        if (ImGui::BeginChild("Asset Contents"))
        {
            if (ImGui::BeginPopupContextWindow("Content Actions", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            { browserMenu(); ImGui::EndPopup(); }
            const int columns =
                m_ContentGrid
                    ? std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / (m_ContentThumbnailSize + 30)))
                    : 1;
            if (ImGui::BeginTable("Asset Tiles", columns, ImGuiTableFlags_SizingStretchSame))
            {
                if (m_AssetSearch.empty() && !typeFilter)
                {
                    std::vector<fs::path> folders;
                    for (fs::directory_iterator it(directory, error), end; it != end && !error; it.increment(error))
                        if (it->is_directory())
                            folders.push_back(it->path());
                    std::sort(folders.begin(), folders.end());
                    for (const auto &folder : folders)
                    {
                        ImGui::TableNextColumn();
                        ImGui::PushID(folder.string().c_str());
                        const auto origin = ImGui::GetCursorScreenPos();
                        const float width = ImGui::GetContentRegionAvail().x;
                        const float height = m_ContentGrid ? m_ContentThumbnailSize + ImGui::GetTextLineHeight() + 12 : ImGui::GetFrameHeight();
                        const bool clicked = ImGui::Selectable("##Folder Tile", m_SelectedContentFolder == folder.string(),
                            ImGuiSelectableFlags_AllowDoubleClick, {width, height});
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", folder.filename().string().c_str());
                        if (ImGui::BeginDragDropTarget())
                        {
                            if (const auto* payload = ImGui::AcceptDragDropPayload("FinalityAsset"); payload && payload->DataSize == sizeof(uint64_t))
                            {
                                const auto id = *static_cast<const uint64_t*>(payload->Data);
                                if (const auto* asset = AssetRegistry::Find(id))
                                    AssetRegistry::Move(id, (folder / fs::path(asset->Path).filename()).lexically_relative(m_Session.ProjectData.GetDirectory()));
                            }
                            ImGui::EndDragDropTarget();
                        }
                        if (clicked)
                        {
                            m_SelectedContentFolder = folder.string();
                            m_Session.SelectedAsset = 0;
                            m_Session.SelectedEntity = entt::null;
                            if (!m_ContentGrid || ImGui::IsMouseDoubleClicked(0)) m_ContentDirectory = folder.string();
                        }
                        if (m_ContentGrid)
                        {
                            const ImVec2 start{origin.x + 4, origin.y + 4};
                            const float size = m_ContentThumbnailSize;
                            auto *draw = ImGui::GetWindowDrawList();
                            const auto color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
                            draw->AddRectFilled({start.x + 5, start.y + size * 0.22f},
                                                {start.x + size * 0.5f, start.y + size * 0.4f}, color, 3);
                            draw->AddRectFilled({start.x + 5, start.y + size * 0.34f},
                                                {start.x + size - 5, start.y + size * 0.82f}, color, 3);
                        }
                        auto* draw = ImGui::GetWindowDrawList();
                        draw->PushClipRect(origin, {origin.x + width, origin.y + height}, true);
                        draw->AddText({origin.x + 4, origin.y + (m_ContentGrid ? m_ContentThumbnailSize + 8 : 3)},
                            ImGui::GetColorU32(ImGuiCol_Text), folder.filename().string().c_str());
                        draw->PopClipRect();
                        ImGui::PopID();
                    }
                }
                std::vector<const AssetRecord *> sorted;
                for (const auto &[id, asset] : AssetRegistry::GetRecords())
                    sorted.push_back(&asset);
                std::sort(sorted.begin(), sorted.end(), [](const auto *a, const auto *b) { return a->Path < b->Path; });
                for (const auto *record : sorted)
                {
                    const auto &asset = *record;
                    const auto id = asset.ID;
                    if (!MatchesSearch(asset.Path, m_AssetSearch))
                        continue;
                    if (m_AssetSearch.empty() && (m_Session.ProjectData.GetDirectory() / asset.Path).parent_path() != directory)
                        continue;
                    if (typeFilter && static_cast<int>(asset.Type) != typeFilter)
                        continue;
                    ImGui::TableNextColumn();
                    ImGui::PushID(std::to_string(id).c_str());
                    const auto origin = ImGui::GetCursorScreenPos();
                    const float width = ImGui::GetContentRegionAvail().x;
                    const float size = m_ContentGrid ? m_ContentThumbnailSize : 26;
                    const float statusHeight = asset.Status == "Ready" ? 0.0f : ImGui::GetTextLineHeightWithSpacing();
                    const float height = (m_ContentGrid ? size + ImGui::GetTextLineHeight() + 12 : 34) + statusHeight;
                    const bool rowClicked = ImGui::Selectable("##Asset Tile", m_Session.SelectedAsset == id,
                        ImGuiSelectableFlags_AllowDoubleClick, {width, height});
                    const bool hovered = ImGui::IsItemHovered();
                    auto drag = [&]() {
                        if (ImGui::BeginDragDropSource())
                        {
                            ImGui::SetDragDropPayload("FinalityAsset", &id, sizeof(id));
                            ImGui::TextUnformatted(asset.Path.c_str());
                            ImGui::EndDragDropSource();
                        }
                    };
                    if (rowClicked)
                    {
                        m_Session.SelectedAsset = id;
                        m_SelectedContentFolder.clear();
                        m_Session.SelectedEntity = entt::null;
                        const auto reference = "asset://" + std::to_string(id);
                        if (asset.Type == ProjectAssetType::Prefab && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        {
                            if (auto* scene = SceneManager::GetInstance().GetActiveScene())
                                m_Session.SelectedEntity = InstantiatePrefab(*scene, AssetRegistry::Resolve(reference)).GetHandle();
                        }
                        if (asset.Type == ProjectAssetType::Script && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                            m_Session.Tools.OpenScript(AssetRegistry::Resolve(reference));
                        if (asset.Type == ProjectAssetType::Scene && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                            openScene = AssetRegistry::Resolve(reference);
                        if (asset.Type == ProjectAssetType::Model && asset.Status == "Ready" &&
                            ImGui::IsMouseDoubleClicked(0))
                            instantiateID = id;
                    }
                    drag();
                    if (hovered)
                        ImGui::SetTooltip("%s\n%s\n%s", asset.Path.c_str(), asset.Status.c_str(),
                                          asset.Diagnostic.c_str());
                    if (ImGui::BeginPopupContextItem("Asset Actions"))
                    {
                        const auto assetPath = fs::path(AssetRegistry::Resolve("asset://" + std::to_string(id)));
                        if (asset.Type == ProjectAssetType::Script && ImGui::MenuItem("Open Script")) m_Session.Tools.OpenScript(assetPath);
                        if (ImGui::MenuItem("Show in Explorer")) ShellExecuteW(nullptr,L"open",assetPath.parent_path().c_str(),nullptr,nullptr,SW_SHOWNORMAL);
                        if (ImGui::MenuItem("Copy Path")) ImGui::SetClipboardText(assetPath.string().c_str());
                        ImGui::Separator();
                        if (asset.Type == ProjectAssetType::Model && asset.Status == "Ready")
                            if (ImGui::MenuItem("Add to Scene"))
                                instantiateID = id;
                        if (ImGui::MenuItem("Reimport"))
                            reimportID = id;
                        if (ImGui::MenuItem("Rename / Move"))
                        {
                            renameID = id;
                            renamePath = asset.Path;
                            moveID = id;
                        }
                        if (ImGui::MenuItem("Duplicate"))
                            duplicateID = id;
                        if (ImGui::MenuItem("Delete..."))
                            deleteID = id;
                        ImGui::EndPopup();
                    }
                    DrawAssetPreviewAt(asset, {origin.x + 4, origin.y + 4}, size);
                    auto* draw = ImGui::GetWindowDrawList();
                    draw->PushClipRect(origin, {origin.x + width, origin.y + height}, true);
                    draw->AddText({origin.x + (m_ContentGrid ? 4 : size + 10),
                        origin.y + (m_ContentGrid ? size + 8 : 8)}, ImGui::GetColorU32(ImGuiCol_Text),
                        fs::path(asset.Path).filename().string().c_str());
                    if (asset.Status != "Ready")
                        draw->AddText({origin.x + (m_ContentGrid ? 4 : size + 10),
                            origin.y + (m_ContentGrid ? size + 8 : 8) + ImGui::GetTextLineHeightWithSpacing()},
                            ImGui::GetColorU32(ImGuiCol_TextDisabled), asset.Status.c_str());
                    draw->PopClipRect();
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
        if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->InnerRect, ImGui::GetID("Prefab Drop Area")))
        {
            if (const auto* payload = ImGui::AcceptDragDropPayload("FinalityEntity"); payload && payload->DataSize == sizeof(entt::entity))
            {
                if (auto* scene = SceneManager::GetInstance().GetActiveScene())
                {
                    const auto entity = *static_cast<const entt::entity*>(payload->Data);
                    auto path = fs::path(m_ContentDirectory) / "New Prefab.fprefab";
                    for (int index = 1; fs::exists(path); ++index) path = fs::path(m_ContentDirectory) / ("New Prefab " + std::to_string(index) + ".fprefab");
                    SavePrefab(*scene, entity, path);
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::EndChild();
        ImGui::EndTable();
    }
    if (moveID)
        ImGui::OpenPopup("Move Asset");
    if (ImGui::BeginPopupModal("Move Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        EditText("Project-relative Path", renamePath);
        if (ImGui::Button("Apply"))
        {
            AssetPreview::Clear();
            if (AssetRegistry::Move(renameID, renamePath))
                ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (deleteID)
    {
        ImGui::OpenPopup("Delete Asset?");
    }
    if (ImGui::BeginPopupModal("Delete Asset?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Delete this asset? Scene references will be marked missing.");
        if (ImGui::Button("Delete"))
        {
            if (const auto *asset = AssetRegistry::Find(deleteID))
            {
                AssetPreview::Clear();
                const auto path = m_Session.ProjectData.GetDirectory() / asset->Path;
                fs::remove(path, error);
                fs::remove(path.string() + ".fmeta", error);
                AssetRegistry::Scan();
            }
            deleteID = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            deleteID = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (duplicateID)
        if (const auto *asset = AssetRegistry::Find(duplicateID))
        {
            const auto source = m_Session.ProjectData.GetDirectory() / asset->Path;
            auto destination = source.parent_path() / (source.stem().string() + " Copy" + source.extension().string());
            if (!fs::exists(destination))
            {
                fs::copy_file(source, destination, error);
                AssetRegistry::Scan();
            }
        }
    if (reimportID)
    {
        AssetPreview::Clear();
        m_AssetInspector.Clear();
        m_Session.Imports.Queue("Reimporting asset", [reimportID] { AssetRegistry::Reimport(reimportID); });
    }
    if (instantiateID)
        PlaceSceneAsset(instantiateID,glm::vec3(0));
    if (!openScene.empty())
        OpenScene(openScene);
    ImGui::End();
}
