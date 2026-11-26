#include "BuildSupport.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace FI::Build
{
    std::string Text(const fs::path& path)
    {
        const auto value = path.generic_u8string();
        return { value.begin(), value.end() };
    }

    fs::path Path(const std::string& text)
    {
        return fs::u8path(text);
    }

    std::string Read(const fs::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input)
        {
            throw std::runtime_error("Cannot read " + Text(path));
        }
        return { (std::istreambuf_iterator<char>(input)), {} };
    }

    void Write(const fs::path& path, const std::string& text)
    {
        if (fs::is_regular_file(path) && Read(path) == text)
        {
            return;
        }
        fs::create_directories(path.parent_path());
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << text;
        if (!output)
        {
            throw std::runtime_error("Cannot write " + Text(path));
        }
    }

    std::string Hash(const std::string& bytes)
    {
        constexpr std::array<uint32_t, 64> constants{
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };
        std::vector<uint8_t> data(bytes.begin(), bytes.end());
        const uint64_t bits = uint64_t(data.size()) * 8;
        data.push_back(0x80);
        while (data.size() % 64 != 56)
        {
            data.push_back(0);
        }
        for (int i = 7; i >= 0; --i)
        {
            data.push_back(uint8_t(bits >> (i * 8)));
        }
        std::array<uint32_t, 8> hash{ 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                      0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
        for (size_t offset = 0; offset < data.size(); offset += 64)
        {
            std::array<uint32_t, 64> words{};
            for (size_t i = 0; i < 16; ++i)
            {
                for (size_t j = 0; j < 4; ++j)
                {
                    words[i] = (words[i] << 8) | data[offset + i * 4 + j];
                }
            }
            for (size_t i = 16; i < 64; ++i)
            {
                const auto x = words[i - 15], y = words[i - 2];
                words[i] = words[i - 16] + (std::rotr(x, 7) ^ std::rotr(x, 18) ^ (x >> 3)) + words[i - 7] +
                           (std::rotr(y, 17) ^ std::rotr(y, 19) ^ (y >> 10));
            }
            auto state = hash;
            for (size_t i = 0; i < 64; ++i)
            {
                const auto [a, b, c, d, e, f, g, h] = state;
                const auto t1 = h + (std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25)) +
                                ((e & f) ^ (~e & g)) + constants[i] + words[i];
                const auto t2 = (std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
                state = { t1 + t2, a, b, c, d + t1, e, f, g };
            }
            for (size_t i = 0; i < 8; ++i)
            {
                hash[i] += state[i];
            }
        }
        std::ostringstream result;
        for (auto word : hash)
        {
            result << std::hex << std::setw(8) << std::setfill('0') << word;
        }
        return result.str();
    }

    std::string HeaderHash(const fs::path& engine)
    {
        const auto source = fs::absolute(engine / "FinalityEngine/src").lexically_normal();
        std::vector<fs::path> files;
        for (const auto& file : fs::recursive_directory_iterator(source))
        {
            if (file.is_regular_file() && (file.path().extension() == ".h" || file.path().extension() == ".inl"))
            {
                files.push_back(file.path());
            }
        }
        std::sort(files.begin(), files.end(),
                  [](const auto& a, const auto& b)
                  {
                      return a.native() < b.native();
                  });
        std::string bytes;
        for (const auto& file : files)
        {
            bytes += "/" + Text(file.lexically_relative(source)) + Read(file);
        }
        return Hash(bytes);
    }

    std::string Json(const YAML::Node& node)
    {
        const auto quoted = [](const std::string& text)
        {
            std::ostringstream out;
            out << '"';
            for (unsigned char c : text)
            {
                if (c == '"' || c == '\\')
                {
                    out << '\\' << c;
                }
                else if (c < 32)
                {
                    out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << unsigned(c);
                }
                else
                {
                    out << c;
                }
            }
            out << '"';
            return out.str();
        };
        if (!node || node.IsNull())
        {
            return "null";
        }
        if (node.IsScalar())
        {
            if (node.Tag() == "!!bool" || node.Tag() == "!!int")
            {
                return node.Scalar();
            }
            return quoted(node.Scalar());
        }
        std::string result = node.IsMap() ? "{" : "[";
        bool first = true;
        for (const auto& item : node)
        {
            if (!first)
            {
                result += ",";
            }
            first = false;
            if (node.IsMap())
            {
                result += quoted(item.first.as<std::string>()) + ":" + Json(item.second);
            }
            else
            {
                result += Json(item);
            }
        }
        return result + (node.IsMap() ? "}" : "]");
    }

    std::string Xml(std::string text)
    {
        const std::pair<std::string, std::string> replacements[] =
        {
            { "&", "&amp;" },
            { "\"", "&quot;" },
            { "<", "&lt;" },
            { ">", "&gt;" }
        };

        for (const auto& [from, to] : replacements)
        {
            for (size_t pos = 0; (pos = text.find(from, pos)) != std::string::npos; pos += to.size())
            {
                text.replace(pos, from.size(), to);
            }
        }
        return text;
    }

    bool Inside(const fs::path& path, const fs::path& parent)
    {
        const auto relative = fs::weakly_canonical(path).lexically_relative(fs::weakly_canonical(parent));
        return !relative.empty() && !relative.is_absolute() && *relative.begin() != "..";
    }

    std::string Name(std::string text)
    {
        for (auto& c : text)
        {
            if (static_cast<unsigned char>(c) < 32 || std::string("<>:\"/\\|?*").find(c) != std::string::npos)
            {
                c = '_';
            }
        }
        while (!text.empty() && (text.back() == ' ' || text.back() == '.'))
        {
            text.pop_back();
        }
        while (!text.empty() && text.front() == ' ')
        {
            text.erase(text.begin());
        }
        return text.empty() ? "FinalityProject" : text;
    }
#ifdef _WIN32
    struct Handle
    {
        HANDLE Value = nullptr;

        ~Handle()
        {
            if (Value && Value != INVALID_HANDLE_VALUE)
            {
                CloseHandle(Value);
            }
        }
    };

    std::wstring Wide(const std::string& text)
    {
        const int size =
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        if (!size && !text.empty())
        {
            throw std::runtime_error("Invalid UTF-8 argument");
        }
        std::wstring result(size, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), result.data(), size);
        return result;
    }

    std::wstring Quote(const std::wstring& text)
    {
        std::wstring result = L"\"";
        size_t slashes = 0;
        for (auto c : text)
        {
            if (c == L'\\')
            {
                ++slashes;
                continue;
            }
            result.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
            result += c;
            slashes = 0;
        }
        result.append(slashes * 2, L'\\');
        return result + L"\"";
    }
#endif
    std::string Run(const std::vector<std::string>& args, const fs::path& cwd)
    {
        if (args.empty())
        {
            throw std::runtime_error("Missing executable");
        }
        std::string output;
        int exitCode = 0;
#ifdef _WIN32
        SECURITY_ATTRIBUTES security{ sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
        Handle read, write, input;
        if (!CreatePipe(&read.Value, &write.Value, &security, 0))
        {
            throw std::runtime_error("Cannot create process output pipe");
        }
        SetHandleInformation(read.Value, HANDLE_FLAG_INHERIT, 0);
        input.Value = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security,
                                  OPEN_EXISTING, 0, nullptr);
        std::wstring command;
        for (const auto& arg : args)
        {
            if (!command.empty())
            {
                command += L' ';
            }
            command += Quote(Wide(arg));
        }
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdOutput = write.Value;
        startup.hStdError = write.Value;
        startup.hStdInput = input.Value;
        PROCESS_INFORMATION info{};
        if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, cwd.c_str(), &startup, &info))
        {
            throw std::runtime_error("Cannot launch " + args[0] + " (Windows error " +
                                     std::to_string(GetLastError()) + ")");
        }
        Handle child{ info.hProcess }, thread{ info.hThread };
        CloseHandle(write.Value);
        write.Value = nullptr;
        char buffer[4096];
        DWORD count = 0;
        while (ReadFile(read.Value, buffer, sizeof(buffer), &count, nullptr) && count)
        {
            output.append(buffer, count);
        }
        WaitForSingleObject(child.Value, INFINITE);
        DWORD status = 0;
        GetExitCodeProcess(child.Value, &status);
        exitCode = int(status);
#else
        int pipeHandles[2];
        if (pipe(pipeHandles))
        {
            throw std::runtime_error("Cannot create process pipe");
        }
        const auto child = fork();
        if (child < 0)
        {
            close(pipeHandles[0]);
            close(pipeHandles[1]);
            throw std::runtime_error("Cannot fork build process");
        }
        if (child == 0)
        {
            close(pipeHandles[0]);
            dup2(pipeHandles[1], STDOUT_FILENO);
            dup2(pipeHandles[1], STDERR_FILENO);
            close(pipeHandles[1]);
            if (chdir(cwd.c_str()))
            {
                _exit(127);
            }
            std::vector<char*> values;
            for (const auto& arg : args)
            {
                values.push_back(const_cast<char*>(arg.c_str()));
            }
            values.push_back(nullptr);
            execvp(values[0], values.data());
            _exit(127);
        }
        close(pipeHandles[1]);
        char buffer[4096];
        ssize_t count;
        while ((count = read(pipeHandles[0], buffer, sizeof(buffer))) > 0)
        {
            output.append(buffer, size_t(count));
        }
        close(pipeHandles[0]);
        int status = 0;
        waitpid(child, &status, 0);
        exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : 128;
#endif
        if (exitCode)
        {
            throw std::runtime_error(output + "\nBuild command failed: " + args[0] + " (" +
                                     std::to_string(exitCode) + ")");
        }
        return output;
    }
} // namespace FI::Build
