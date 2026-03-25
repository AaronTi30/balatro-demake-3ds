#include "core/AssetPath.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void fail(const std::string& message) {
    std::cerr << message << '\n';
    std::exit(1);
}

void expect(bool condition, const std::string& label) {
    if (!condition) {
        fail(label);
    }
}

void expectEqual(const std::filesystem::path& actual,
                 const std::filesystem::path& expected,
                 const std::string& label) {
    if (actual != expected) {
        std::ostringstream oss;
        oss << label << ": expected \"" << expected.string()
            << "\", got \"" << actual.string() << "\"";
        fail(oss.str());
    }
}

void testResolveAssetPathFindsAssetsFromRepoRoot() {
    const std::filesystem::path buildDir = std::filesystem::current_path();
    const std::filesystem::path repoRoot = buildDir.parent_path();
    const std::filesystem::path expected = std::filesystem::weakly_canonical(
        repoRoot / "assets/textures/card_base.png"
    );

    expect(std::filesystem::exists(expected),
           "test fixture requires assets/textures/card_base.png in repo root");

    const std::filesystem::path resolved = resolveAssetPath(
        "assets/textures/card_base.png",
        repoRoot,
        buildDir
    );

    expectEqual(resolved, expected,
                "resolver should keep repo-root asset path when cwd is repo root");
}

void testResolveAssetPathFindsAssetsFromBuildDirectory() {
    const std::filesystem::path buildDir = std::filesystem::current_path();
    const std::filesystem::path repoRoot = buildDir.parent_path();
    const std::filesystem::path expected = std::filesystem::weakly_canonical(
        repoRoot / "assets/textures/card_base.png"
    );

    expect(std::filesystem::exists(expected),
           "test fixture requires assets/textures/card_base.png in repo root");

    const std::filesystem::path resolved = resolveAssetPath(
        "assets/textures/card_base.png",
        buildDir,
        buildDir
    );

    expectEqual(resolved, expected,
                "resolver should find assets when launched from build directory");
}

} // namespace

int main() {
    testResolveAssetPathFindsAssetsFromRepoRoot();
    testResolveAssetPathFindsAssetsFromBuildDirectory();

    std::cout << "AssetPath tests passed\n";
    return 0;
}
