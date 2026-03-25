#pragma once

#include <filesystem>

std::filesystem::path resolveAssetPath(const std::filesystem::path& relativePath,
                                       const std::filesystem::path& currentDir,
                                       const std::filesystem::path& executableDir);
