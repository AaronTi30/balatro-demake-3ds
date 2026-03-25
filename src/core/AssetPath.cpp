#include "AssetPath.h"

#include <array>

namespace {

std::filesystem::path findAssetUnderRoot(const std::filesystem::path& root,
                                         const std::filesystem::path& relativePath) {
    std::filesystem::path candidateRoot = root;
    for (int depth = 0; depth < 6; ++depth) {
        const std::filesystem::path candidate = candidateRoot / relativePath;
        if (std::filesystem::exists(candidate)) {
            return std::filesystem::weakly_canonical(candidate);
        }

        if (!candidateRoot.has_parent_path()) {
            break;
        }
        const std::filesystem::path parent = candidateRoot.parent_path();
        if (parent == candidateRoot) {
            break;
        }
        candidateRoot = parent;
    }

    return {};
}

} // namespace

std::filesystem::path resolveAssetPath(const std::filesystem::path& relativePath,
                                       const std::filesystem::path& currentDir,
                                       const std::filesystem::path& executableDir) {
    if (relativePath.is_absolute() && std::filesystem::exists(relativePath)) {
        return std::filesystem::weakly_canonical(relativePath);
    }

    for (const auto& root : std::array<std::filesystem::path, 2>{ currentDir, executableDir }) {
        const std::filesystem::path resolved = findAssetUnderRoot(root, relativePath);
        if (!resolved.empty()) {
            return resolved;
        }
    }

    return relativePath;
}
