#pragma once
#include <filesystem>
#include <Project/PlayerRuntime.h>
#include <yaml-cpp/yaml.h>
/** @brief Bake executable identity, multi-resolution icons and the minimal runtime payload. Editor YAML is never copied. */
void BakeExecutableResources(const std::filesystem::path& executable, const std::filesystem::path& project,
                             const std::filesystem::path& bin, const YAML::Node& request,
                             const FINALITY::PlayerRuntime& runtime);
