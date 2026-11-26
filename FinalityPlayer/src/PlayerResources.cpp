#include "PlayerResources.h"
#include <Core/Process.h>
#include <stdexcept>
#ifdef _WIN32
#define UNICODE
#define NOMINMAX
#include <Windows.h>
#include <winver.h>
#endif
FINALITY::PlayerRuntime ReadPlayerRuntime()
{
#ifdef _WIN32
    const auto module = GetModuleHandleW(nullptr);
    const auto resource = FindResourceW(module, MAKEINTRESOURCEW(FINALITY::PlayerRuntimeResource), RT_RCDATA);
    if (!resource)
    {
        throw std::runtime_error("This player has not been baked. Export a game from the editor first.");
    }
    const auto data = LoadResource(module, resource);
    const auto* bytes = static_cast<const uint8_t*>(LockResource(data));
    if (!bytes)
    {
        throw std::runtime_error("Cannot load embedded game data");
    }
    return FINALITY::DecodePlayerRuntime({ bytes, SizeofResource(module, resource) });
#else
    throw std::runtime_error("Embedded player resources are currently implemented for Windows exports");
#endif
}

std::string ReadPlayerProductName()
{
#ifdef _WIN32
    const auto path = FINALITY::Process::ExecutablePath();
    const auto size = GetFileVersionInfoSizeW(path.c_str(), nullptr);
    std::vector<uint8_t> data(size);
    if (size && GetFileVersionInfoW(path.c_str(), 0, size, data.data()))
    {
        wchar_t* name = nullptr;
        UINT length = 0;
        if (VerQueryValueW(data.data(), L"\\StringFileInfo\\040904b0\\ProductName", reinterpret_cast<void**>(&name), &length) && length > 1)
        {
            const int count = WideCharToMultiByte(CP_UTF8, 0, name, length - 1, nullptr, 0, nullptr, nullptr);
            std::string result(count, '\0');
            WideCharToMultiByte(CP_UTF8, 0, name, length - 1, result.data(), count, nullptr, nullptr);
            return result;
        }
    }
#endif
    return "Finality Game";
}
