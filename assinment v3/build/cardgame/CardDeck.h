#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <learnopengl/shader_m.h>

#include "tools/Cube.h"
#include "tools/Polygon.h"
#include "TextureLoader.h"

#include <memory>
#include <random>
#include <string>
#include <vector>

struct CardItem {
    int id = 0;
    std::string characterName;
    GLuint frontTexture = 0;

    bool inDeck = true;
    bool isAnimating = false;
    float moveProgress = 0.0f; // [0.0, 1.0]

    glm::vec3 startPos{0.0f};
    glm::vec3 targetPos{0.0f};
    glm::vec3 currentPos{0.0f};
};

class CardDeck {
public:
    static constexpr int kInitialCards = 5;
    static constexpr float kCardSize = 0.56f;
    static constexpr float kCardThickness = 0.016f;
    static constexpr float kTableSurfaceY = 1.010f;

    CardDeck() = default;
    ~CardDeck();

    void init(TextureLoader& texLoader);

    // Triggered by pressing 'G' (works identically in Camera 1 & Camera 2; 6th+ press does nothing)
    void drawRandomCard();

    void update(float deltaTime, float standFactor);
    void render(Shader& shader) const;

    int remainingCount() const { return static_cast<int>(remainingIndices_.size()); }
    int playedCount() const { return playedCount_; }

private:
    static constexpr float kDeckRowZ = 1.26f;
    static constexpr float kPlayedRowZ = 0.22f;

    static float slotX(int slotIndex);

    std::unique_ptr<PolygonWithTexture> cardFrontQuad_;
    std::unique_ptr<PolygonWithTexture> cardBackQuad_;
    std::unique_ptr<Cube> cardSlab_;

    GLuint backTexture_ = 0;
    std::vector<CardItem> cards_;
    std::vector<int> remainingIndices_;
    int playedCount_ = 0;
    float currentTiltDeg_ = 0.0f;
    std::mt19937 rng_;
};
