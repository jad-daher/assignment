#include "TextureLoader.h"

#include <stb_image.h>
#include <iostream>
#include <vector>

TextureLoader::~TextureLoader() {
    releaseAll();
}

GLuint TextureLoader::load(const std::string& path, bool repeatWrap,
                           glm::vec3 fallbackA, glm::vec3 fallbackB) {
    auto it = cache_.find(path);
    if (it != cache_.end()) {
        return it->second;
    }

    stbi_set_flip_vertically_on_load(true);
    int width = 0, height = 0, nrChannels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    GLuint texId = 0;
    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 4) format = GL_RGBA;
        else if (nrChannels == 1) format = GL_RED;

        texId = uploadTexture(data, width, height, format, repeatWrap);
        stbi_image_free(data);
        std::cout << "[TextureLoader] Loaded " << path << " (" << width << "x" << height << ")\n";
    } else {
        std::cerr << "[TextureLoader] Warning: failed to load '" << path
                  << "'. Using fallback checkerboard texture.\n";
        texId = createFallback(fallbackA, fallbackB, repeatWrap);
    }

    cache_[path] = texId;
    return texId;
}

void TextureLoader::releaseAll() {
    for (auto& kv : cache_) {
        if (kv.second != 0) {
            glDeleteTextures(1, &kv.second);
        }
    }
    cache_.clear();
}

GLuint TextureLoader::uploadTexture(const unsigned char* data, int width, int height,
                                    GLenum format, bool repeatWrap) {
    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    GLint wrap = repeatWrap ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    return texId;
}

GLuint TextureLoader::createFallback(glm::vec3 colorA, glm::vec3 colorB, bool repeatWrap) {
    constexpr int kSize = 128;
    std::vector<unsigned char> pixels(kSize * kSize * 3);
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            bool border = (x < 6 || x >= kSize - 6 || y < 6 || y >= kSize - 6);
            bool checker = ((x / 16) + (y / 16)) % 2 == 0;
            glm::vec3 col = border ? glm::vec3(0.12f) : (checker ? colorA : colorB);
            int idx = (y * kSize + x) * 3;
            pixels[idx + 0] = static_cast<unsigned char>(glm::clamp(col.r, 0.0f, 1.0f) * 255.0f);
            pixels[idx + 1] = static_cast<unsigned char>(glm::clamp(col.g, 0.0f, 1.0f) * 255.0f);
            pixels[idx + 2] = static_cast<unsigned char>(glm::clamp(col.b, 0.0f, 1.0f) * 255.0f);
        }
    }
    return uploadTexture(pixels.data(), kSize, kSize, GL_RGB, repeatWrap);
}
