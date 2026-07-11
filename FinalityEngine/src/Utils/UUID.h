#pragma once
#include <cstdint>
#include <xhash>

#include <Core/Core.h>

namespace FINALITY
{
    class FAPI UUID
    {
    public:
        UUID();
        UUID(uint64_t uuid);
        UUID(const UUID&) = default;

        operator uint64_t() const { return m_UUID; }

    private:
        uint64_t m_UUID;
    };
}

namespace std {
    template <typename T> struct hash;
    template<>
    struct hash<FINALITY::UUID>
    {
        std::size_t operator()(const FINALITY::UUID& uuid) const
        {
            return std::hash<uint64_t>()((uint64_t)uuid);
        }
    };
}
