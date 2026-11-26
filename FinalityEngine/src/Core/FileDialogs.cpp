#include "FileDialogs.h"
#include <cstdio>
#include <array>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shobjidl.h>
#include <wrl/client.h>
#include <vector>
#include <filesystem>
#else
#include <cstdlib>
#endif

#ifdef _WIN32
namespace
{
    std::wstring Wide(const char* text)
    {
        if (!text || !*text)
        {
            return {};
        }
        int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, nullptr, 0);
        const auto encoding = length ? CP_UTF8 : CP_ACP;
        if (!length)
        {
            length = MultiByteToWideChar(encoding, 0, text, -1, nullptr, 0);
        }
        std::wstring value(length, L'\0');
        if (length)
        {
            MultiByteToWideChar(encoding, 0, text, -1, value.data(), length);
        }
        if (!value.empty())
        {
            value.pop_back();
        }
        return value;
    }

    std::string Pick(bool folder, bool save, const char* filter, const char* defaultName)
    {
        const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(initialized))
        {
            return {};
        }

        struct Apartment
        {
            ~Apartment()
            {
                CoUninitialize();
            }
        } apartment;

        Microsoft::WRL::ComPtr<IFileDialog> dialog;
        const auto clsid = save ? __uuidof(FileSaveDialog) : __uuidof(FileOpenDialog);
        if (FAILED(CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(dialog.GetAddressOf()))))
        {
            return {};
        }
        FILEOPENDIALOGOPTIONS options{};
        if (FAILED(dialog->GetOptions(&options)))
        {
            return {};
        }
        options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR;
        options |= folder ? FOS_PICKFOLDERS : save ? FOS_OVERWRITEPROMPT
                                                   : FOS_FILEMUSTEXIST;
        if (FAILED(dialog->SetOptions(options)))
        {
            return {};
        }
        dialog->SetTitle(folder ? L"Select Folder" : save ? L"Save File"
                                                          : L"Open File");
        if (folder)
        {
            dialog->SetOkButtonLabel(L"Select Folder");
        }
        std::vector<std::pair<std::wstring, std::wstring>> types;
        if (filter && !folder)
        {
            for (const char* item = filter; *item;)
            {
                const char* pattern = item + std::char_traits<char>::length(item) + 1;
                if (!*pattern)
                {
                    break;
                }
                types.emplace_back(Wide(item), Wide(pattern));
                item = pattern + std::char_traits<char>::length(pattern) + 1;
            }
        }
        std::vector<COMDLG_FILTERSPEC> specifications;
        for (const auto& type : types)
        {
            specifications.push_back({ type.first.c_str(), type.second.c_str() });
        }
        if (!specifications.empty())
        {
            if (FAILED(dialog->SetFileTypes(static_cast<UINT>(specifications.size()), specifications.data())))
            {
                return {};
            }
            dialog->SetFileTypeIndex(1);
            auto pattern = types.front().second;
            if (save && pattern.starts_with(L"*."))
            {
                auto extension = pattern.substr(2, pattern.find(L';') == std::wstring::npos ? std::wstring::npos : pattern.find(L';') - 2);
                if (extension.find_first_of(L"*?") == std::wstring::npos)
                {
                    dialog->SetDefaultExtension(extension.c_str());
                }
            }
        }
        if (defaultName && *defaultName)
        {
            const std::filesystem::path path(Wide(defaultName));
            dialog->SetFileName(path.filename().c_str());
            if (path.has_parent_path() && std::filesystem::is_directory(path.parent_path()))
            {
                Microsoft::WRL::ComPtr<IShellItem> initial;
                if (SUCCEEDED(SHCreateItemFromParsingName(path.parent_path().c_str(), nullptr, IID_PPV_ARGS(initial.GetAddressOf()))))
                {
                    dialog->SetFolder(initial.Get());
                }
            }
        }
        if (FAILED(dialog->Show(GetActiveWindow())))
        {
            return {};
        }
        Microsoft::WRL::ComPtr<IShellItem> selected;
        if (FAILED(dialog->GetResult(selected.GetAddressOf())))
        {
            return {};
        }
        PWSTR path = nullptr;
        if (FAILED(selected->GetDisplayName(SIGDN_FILESYSPATH, &path)))
        {
            return {};
        }
        const int length = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
        std::string result(length, '\0');
        if (length)
        {
            WideCharToMultiByte(CP_UTF8, 0, path, -1, result.data(), length, nullptr, nullptr);
        }
        CoTaskMemFree(path);
        if (!result.empty())
        {
            result.pop_back();
        }
        return result;
    }
} // namespace
#endif

std::string FINALITY::FileDialogs::OpenFile(const char* filter)
{
#ifdef _WIN32
    return Pick(false, false, filter, nullptr);
#else
    // LINUX
    std::array<char, 4096> buffer{};

    FILE* pipe = popen(
        "zenity --file-selection --file-filter='Scene files | *.scene'",
        "r");

    if (!pipe)
    {
        return {};
    }

    if (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
    {
        pclose(pipe);

        std::string path = buffer.data();

        if (!path.empty() && path.back() == '\n')
        {
            path.pop_back();
        }

        return path;
    }

    pclose(pipe);
    return {};
#endif
}

std::string FINALITY::FileDialogs::SaveFile(const char* filter, const char* defaultName)
{
#ifdef _WIN32
    return Pick(false, true, filter, defaultName);
#else
    // LINUX
    std::string command =
        "zenity --file-selection --save "
        "--confirm-overwrite "
        "--filename=\"" +
        std::string(defaultName) + "\" "
                                   "--file-filter='Scene files | *.scene'";

    std::array<char, 4096> buffer{};

    FILE* pipe = popen(command.c_str(), "r");

    if (!pipe)
    {
        return {};
    }

    if (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
    {
        pclose(pipe);

        std::string path = buffer.data();

        if (!path.empty() && path.back() == '\n')
        {
            path.pop_back();
        }

        return path;
    }

    pclose(pipe);
    return {};
#endif
}

std::string FINALITY::FileDialogs::OpenFolder()
{
#ifdef _WIN32
    return Pick(true, false, nullptr, nullptr);
#else
    std::array<char, 4096> buffer{};
    FILE* pipe = popen("zenity --file-selection --directory", "r");
    if (!pipe)
    {
        return {};
    }
    std::string path;
    if (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
    {
        path = buffer.data();
    }
    pclose(pipe);
    if (!path.empty() && path.back() == '\n')
    {
        path.pop_back();
    }
    return path;
#endif
}
