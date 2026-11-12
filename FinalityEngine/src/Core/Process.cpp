#include "Process.h"
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN32
namespace
{
    struct Handle
    {
        HANDLE Value = nullptr;
        ~Handle() { if (Value && Value != INVALID_HANDLE_VALUE) CloseHandle(Value); }
    };
    std::wstring Quote(const std::wstring& value)
    {
        std::wstring result = L"\"";
        size_t slashes = 0;
        for (const auto c : value)
        {
            if (c == L'\\') { ++slashes; continue; }
            result.append(c == L'\"' ? slashes * 2 + 1 : slashes,L'\\');
            result += c; slashes = 0;
        }
        result.append(slashes * 2,L'\\'); result += L'\"';
        return result;
    }
}
#endif

FINALITY::ProcessResult FINALITY::Process::Run(const std::vector<std::wstring>& arguments, const std::filesystem::path& directory)
{
    ProcessResult result;
#ifdef _WIN32
    if (arguments.empty()) return result;
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};
    Handle read,write,input;
    if (!CreatePipe(&read.Value,&write.Value,&security,0)) return result;
    SetHandleInformation(read.Value,HANDLE_FLAG_INHERIT,0);
    input.Value = CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,nullptr);
    std::wstring command;
    for (const auto& argument : arguments) { if (!command.empty()) command += L' '; command += Quote(argument); }
    STARTUPINFOW startup{}; startup.cb = sizeof(startup); startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = write.Value; startup.hStdError = write.Value; startup.hStdInput = input.Value;
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr,command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,directory.c_str(),&startup,&process))
    { result.Output = "Cannot start build process (Windows error " + std::to_string(GetLastError()) + ")"; return result; }
    Handle child{process.hProcess},thread{process.hThread};
    CloseHandle(write.Value); write.Value = nullptr;
    char buffer[4096]; DWORD count = 0;
    while (ReadFile(read.Value,buffer,sizeof(buffer),&count,nullptr) && count)
        if (result.Output.size() < 2 * 1024 * 1024) result.Output.append(buffer,count);
    WaitForSingleObject(child.Value,INFINITE);
    DWORD code = 0; GetExitCodeProcess(child.Value,&code); result.ExitCode = static_cast<int>(code);
#else
    result.Output = "Project script builds require the Windows build workflow";
#endif
    return result;
}

std::filesystem::path FINALITY::Process::ExecutablePath()
{
#ifdef _WIN32
    std::wstring value(32768,L'\0');
    const auto size = GetModuleFileNameW(nullptr,value.data(),static_cast<DWORD>(value.size()));
    if (!size || size == value.size()) return {};
    value.resize(size); return value;
#else
    return std::filesystem::read_symlink("/proc/self/exe");
#endif
}
