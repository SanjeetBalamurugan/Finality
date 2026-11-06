#include "ShaderCompiler.h"
#include <Core/Log.h>
#include <filesystem>
#include <string>
#include <stdexcept>

#ifdef _WIN32
#include <process.h>
#include <cstdlib>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace
{
    int RunCompiler(const std::string& source, const std::string& output)
    {
        const std::string include = std::filesystem::path(source).parent_path().string();
        const char* argv[] = {"glslc", "-I", include.c_str(), source.c_str(), "-o", output.c_str(), nullptr};
#ifdef _WIN32
        const int status = _spawnvp(_P_WAIT, "glslc", argv);
        if (status != -1) return status;
        if (auto sdk = std::getenv("VULKAN_SDK"))
        {
            const std::string exe = (std::filesystem::path(sdk) / "Bin" / "glslc.exe").string();
            return _spawnv(_P_WAIT, exe.c_str(), argv);
        }
        return -1;
#else
        const pid_t child = fork();
        if (child == -1) return -1;
        if (child == 0)
        {
            execvp("glslc", const_cast<char* const*>(argv));
            _exit(127);
        }
        int status = 0;
        if (waitpid(child, &status, 0) < 0 || !WIFEXITED(status)) return -1;
        return WEXITSTATUS(status);
#endif
    }
}

bool FINALITY::ShaderCompiler::IsSourceShader(const std::string& path)
{
    const auto extension = std::filesystem::path(path).extension().string();
    return extension == ".vert" || extension == ".frag" ||
           extension == ".comp" || extension == ".geom" ||
           extension == ".tesc" || extension == ".tese";
}

bool FINALITY::ShaderCompiler::EnsureCompiled(const std::string& sourcePath, bool force)
{
    namespace fs = std::filesystem;
    if (!IsSourceShader(sourcePath))
    {
        FI_CORE_ERROR("Shader source extension is not supported: {}", sourcePath);
        return false;
    }
    const fs::path source(sourcePath);
    if (!fs::exists(source))
    {
        FI_CORE_ERROR("Shader source was not found: {}", sourcePath);
        return false;
    }
    fs::path binary = source;
    binary += ".spv";
    if (!force && fs::exists(binary) &&
        fs::last_write_time(binary) >= fs::last_write_time(source))
        return true;

    fs::path temporary = binary;
    temporary += ".tmp";
    const int result = RunCompiler(source.string(), temporary.string());
    if (result != 0 || !fs::exists(temporary))
    {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        FI_CORE_ERROR("glslc failed (exit {}): {}", result, sourcePath);
        return false;
    }
    std::error_code error;
    if (fs::exists(binary))
    {
        fs::path backup = binary;
        backup += ".backup";
        fs::remove(backup, error);
        error.clear();
        fs::rename(binary, backup, error);
        if (error)
        {
            FI_CORE_ERROR("Cannot back up shader cache '{}': {}", binary.string(), error.message());
            return false;
        }
        fs::rename(temporary, binary, error);
        if (error)
        {
            std::error_code ignored;
            fs::rename(backup, binary, ignored);
            FI_CORE_ERROR("Cannot update shader cache '{}': {}", binary.string(), error.message());
            return false;
        }
        fs::remove(backup, error);
    }
    else
    {
        fs::rename(temporary, binary, error);
        if (error)
        {
            FI_CORE_ERROR("Cannot save shader cache '{}': {}", binary.string(), error.message());
            return false;
        }
    }
    FI_CORE_INFO("Compiled GLSL to SPIR-V: {}", binary.string());
    return true;
}
