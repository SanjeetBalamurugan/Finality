/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for AssetChangeMonitor.h.
 */
#pragma once
#include <filesystem>
#include <future>
#include <vector>
#include <chrono>
#include <algorithm>

namespace FINALITY
{
    /** @todo TODO: Document AssetChangeMonitor class semantics, ownership and usage. */
    class AssetChangeMonitor
    {
        /** @todo TODO: Document Stamp struct semantics, ownership and usage. */
        struct Stamp
        {
            std::filesystem::path Path;
            std::filesystem::file_time_type Time;
            uintmax_t Size;
            bool operator==(const Stamp&) const = default;
        };

        /** @todo TODO: Document Result struct semantics, ownership and usage. */
        struct Result
        {
            std::filesystem::path Root;
            std::vector<Stamp> Files;
            bool Valid = true;
        };

        std::filesystem::path m_Root;
        std::vector<Stamp> m_Previous;
        std::future<Result> m_Work;
        bool m_Initialized = false;
        std::chrono::steady_clock::time_point m_Next{};

    public:
        bool Poll(const std::filesystem::path& root, float interval)
        {
            using namespace std::chrono;
            if (root != m_Root)
            {
                m_Root = root;
                m_Previous.clear();
                m_Initialized = false;
                m_Next = {};
            }
            bool changed = false;
            if (m_Work.valid() && m_Work.wait_for(seconds(0)) == std::future_status::ready)
            {
                auto result = m_Work.get();
                if (result.Root == m_Root && result.Valid)
                {
                    changed = m_Initialized && result.Files != m_Previous;
                    m_Previous = std::move(result.Files);
                    m_Initialized = true;
                }
            }
            const auto now = steady_clock::now();
            if (!root.empty() && !m_Work.valid() && now >= m_Next)
            {
                m_Next = now + duration_cast<steady_clock::duration>(duration<float>(std::max(interval, 0.25f)));
                m_Work = std::async(std::launch::async, [root]
                                    {
                                        Result result;
                                        result.Root = root;
                                        try
                                        {
                                            for (const auto& entry : std::filesystem::recursive_directory_iterator(root, std::filesystem::directory_options::skip_permission_denied))
                                            {
                                                if (!entry.is_regular_file())
                                                {
                                                    continue;
                                                }
                                                const auto extension = entry.path().extension();
                                                if (extension == ".spv" || extension == ".tmp" || extension == ".ini")
                                                {
                                                    continue;
                                                }
                                                result.Files.push_back({ entry.path(), entry.last_write_time(), entry.file_size() });
                                            }
                                            std::sort(result.Files.begin(), result.Files.end(), [](const Stamp& a, const Stamp& b)
                                                      {
                                                          return a.Path < b.Path;
                                                      });
                                        }
                                        catch (const std::filesystem::filesystem_error&)
                                        {
                                            result.Valid = false;
                                        }
                                        return result;
                                    });
            }
            return changed;
        }
    };
} // namespace FINALITY
