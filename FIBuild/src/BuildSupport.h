#pragma once
/** @file
 * @brief Shared support for native builds, IDE generation and engine build metadata.
 */
#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace FI::Build
{
namespace fs = std::filesystem;
/** @brief Parsed command-line switches; paths are encoded as UTF-8. */
using Options = std::map<std::string, std::string>;
std::string Read(const fs::path &path);
void Write(const fs::path &path, const std::string &text);
std::string Text(const fs::path &path);
fs::path Path(const std::string &text);
std::string Hash(const std::string &bytes);
std::string HeaderHash(const fs::path &engine);
/** @brief Run an argument vector without a shell and capture output; throw on failure. */
std::string Run(const std::vector<std::string> &args, const fs::path &cwd);
std::string Json(const YAML::Node &node);
std::string Xml(std::string text);
bool Inside(const fs::path &path, const fs::path &parent);
std::string Name(std::string text);
/** @brief Installed compiler, SDK and build tool matching an engine's recorded ABI inputs. */
struct Toolchain
{
    fs::path Compiler, SDK, BuildTool;
    std::string Version, Toolset, SDKVersion, Action;
};
Toolchain FindToolchain(const fs::path &engine, const fs::path &bin, const std::string &config);
void GenerateIDE(const fs::path &root, const fs::path &engine, const fs::path &bin, const fs::path &source,
                 const std::string &name, const std::string &config, const Toolchain &tools);
void Native(const Options &options);
void BuildInfo(const Options &options);
void Dependencies(const Options &options);
}
