#include "AssetPath.h"

#include <array>
#include <string>

#ifndef N3DS
#include <SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

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

#ifndef N3DS
SDL_Texture* sdlLoadTexture(SDL_Renderer* renderer, const std::filesystem::path& assetRelativePath) {
    const std::filesystem::path currentDir = std::filesystem::current_path();
    char* rawBasePath = SDL_GetBasePath();
    const std::filesystem::path executableDir =
        rawBasePath ? std::filesystem::path(rawBasePath) : currentDir;
    if (rawBasePath) {
        SDL_free(rawBasePath);
    }

    const std::filesystem::path resolvedPath =
        resolveAssetPath(assetRelativePath, currentDir, executableDir);
    int width = 0;
    int height = 0;
    int channels = 0;
    const std::string pathString = resolvedPath.string();
    unsigned char* pixels = stbi_load(pathString.c_str(), &width, &height, &channels, 4);
    if (!pixels) {
        return nullptr;
    }

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
        pixels, width, height, 32, 4 * width, SDL_PIXELFORMAT_RGBA32
    );
    if (!surface) {
        stbi_image_free(pixels);
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    stbi_image_free(pixels);
    return texture;
}
#endif
