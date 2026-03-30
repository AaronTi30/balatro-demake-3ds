#pragma once

#include <filesystem>

std::filesystem::path resolveAssetPath(const std::filesystem::path& relativePath,
                                       const std::filesystem::path& currentDir,
                                       const std::filesystem::path& executableDir);

#ifndef N3DS
struct SDL_Renderer;
struct SDL_Texture;

SDL_Texture* sdlLoadTexture(SDL_Renderer* renderer, const std::filesystem::path& assetRelativePath);
#endif
