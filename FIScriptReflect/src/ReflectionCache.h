/**
 * @file
 * @ingroup tools
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for ReflectionCache.h.
 */
#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

/** @brief Generated reflection text, script count and dependency stamps for one translation unit. */
struct CachedReflection
{
    std::string Text;
    size_t Scripts = 0;
    bool Legacy = false;
    std::vector<std::filesystem::path> Dependencies;
    std::vector<std::string> Stamps;
};

/** @todo TODO: Document ReflectionCache class semantics, ownership and usage. */
class ReflectionCache
{
public:
    std::filesystem::path Directory;
    std::string Context;

    static std::string Key(const std::filesystem::path& path)
    {
        uint64_t value = 14695981039346656037ull;
        for (unsigned char byte : path.generic_string())
        {
            value = (value ^ byte) * 1099511628211ull;
        }
        return std::to_string(value);
    }

    static std::string Stamp(const std::filesystem::path& path)
    {
        return (std::filesystem::is_directory(path) ? "directory"
                                                    : std::to_string(std::filesystem::file_size(path))) +
               ":" + std::to_string(std::filesystem::last_write_time(path).time_since_epoch().count());
    }

    std::optional<CachedReflection> Read(const std::filesystem::path& source) const
    {
        try
        {
            std::ifstream input(Directory / Key(source), std::ios::binary);
            std::string context, path, stamp;
            CachedReflection result;
            size_t count = 0;
            if (!(input >> std::quoted(context)) || context != Context || !(input >> result.Scripts >> result.Legacy >> count) || count > 100000)
            {
                return {};
            }
            for (size_t i = 0; i < count; ++i)
            {
                if (!(input >> std::quoted(path) >> std::quoted(stamp)) || Stamp(path) != stamp)
                {
                    return {};
                }
                result.Dependencies.emplace_back(path);
                result.Stamps.push_back(stamp);
            }
            if (!(input >> std::quoted(result.Text)))
            {
                return {};
            }
            return result;
        }
        catch (const std::exception&)
        {
            return {};
        }
    }

    void Write(const std::filesystem::path& source, const CachedReflection& value) const
    {
        std::filesystem::create_directories(Directory);
        const auto path = Directory / Key(source);
        const auto temporary = path.string() + ".tmp";
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        output << std::quoted(Context) << '\n'
               << value.Scripts << ' ' << value.Legacy << ' ' << value.Dependencies.size() << '\n';
        for (size_t i = 0; i < value.Dependencies.size(); ++i)
        {
            output << std::quoted(value.Dependencies[i].generic_string()) << ' '
                   << std::quoted(value.Stamps.at(i)) << '\n';
        }
        output << std::quoted(value.Text);
        output.close();
        if (!output)
        {
            throw std::runtime_error("Cannot write reflection cache");
        }
        std::filesystem::remove(path);
        std::filesystem::rename(temporary, path);
    }
};
