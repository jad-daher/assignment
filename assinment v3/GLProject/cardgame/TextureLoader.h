#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class TextureLoader {
public:
    TextureLoader() = default;
    ~TextureLoader();

    TextureLoader(const TextureLoader&) = delete;
    TextureLoader& operator=(const TextureLoader&) = delete;

    // Loads an image via stb_image (compiled in OpenGL.SharedModule) with safe fallback
    GLuint load(const std::string& path,
                bool repeatWrap = true,
                glm::vec3 fallbackA = glm::vec3(0.75f),
                glm::vec3 fallbackB = glm::vec3(0.35f));

    void releaseAll();

private:
    std::unordered_map<std::string, GLuint> cache_;

    static GLuint uploadTexture(const unsigned char* data, int width, int height,
                                GLenum format, bool repeatWrap);
    static GLuint createFallback(glm::vec3 colorA, glm::vec3 colorB, bool repeatWrap);
};
