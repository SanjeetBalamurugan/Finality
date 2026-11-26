/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for HistoryCheckpoint.h.
 */
#pragma once

namespace FINALITY
{
    /** @todo TODO: Document HistoryCheckpoint class semantics, ownership and usage. */
    class HistoryCheckpoint
    {
        bool m_Requested = true;
        bool m_Interacting = false;

    public:
        void Request()
        {
            m_Requested = true;
        }

        void Completed()
        {
            m_Requested = false;
        }

        bool Update(bool interacting, bool actionCompleted = false)
        {
            m_Requested |= actionCompleted;
            if (interacting)
            {
                m_Interacting = true;
                return false;
            }
            const bool checkpoint = m_Requested || m_Interacting;
            m_Requested = m_Interacting = false;
            return checkpoint;
        }
    };
} // namespace FINALITY
