#pragma once
#include <functional>
#include <vector>

namespace FINALITY
{
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
            for (auto it = m_Deletions.rbegin(); it != m_Deletions.rend(); ++it)
            {
                (*it)();
            }
            m_Deletions.clear();
        }

    private:
        std::vector<std::function<void()>> m_Deletions;
    };
}
