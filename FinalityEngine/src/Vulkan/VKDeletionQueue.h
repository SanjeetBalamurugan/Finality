/**
 * @file
 * @ingroup vulkan
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for VKDeletionQueue.h.
 */
#pragma once
#include <functional>
#include <vector>

namespace FINALITY
{
    /** @todo TODO: Document VKDeletionQueue class semantics, ownership and usage. */
    class VKDeletionQueue
    {
    public:
        VKDeletionQueue() = default;
        void Push(std::function<void()>&& cleanupFunc)
        {
            m_Deletions.push_back(std::move(cleanupFunc));
        }

        void Flush()
        {
            while (!m_Deletions.empty())
            {
                auto pending = std::move(m_Deletions);
                m_Deletions.clear();
                for (auto it = pending.rbegin(); it != pending.rend(); ++it) (*it)();
            }
        }

    private:
        std::vector<std::function<void()>> m_Deletions;
    };
}
