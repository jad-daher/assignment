#include "CardDeck.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

namespace {
constexpr float kPi = 3.1415926535f;
}

CardDeck::~CardDeck() {
    if (cardFrontQuad_) cardFrontQuad_->deleteBuffers();
    if (cardBackQuad_) cardBackQuad_->deleteBuffers();
}

float CardDeck::slotX(int slotIndex) {
    return (slotIndex - 2) * 0.68f;
}

void CardDeck::init(TextureLoader& texLoader) {
    const float hs = kCardSize * 0.5f;
    const float t = kCardThickness;

    // Square card front face pivoted at bottom edge z = 0 (extends toward -Z).
    // At rotX = 0 deg: lies horizontally on tabletop (+Y normal).
    // At rotX = +90 deg: stands vertically on bottom edge facing player (+Z normal).
    std::vector<glm::vec3> frontVerts = {
        {-hs, t + 0.001f,  0.0f},
        { hs, t + 0.001f,  0.0f},
        { hs, t + 0.001f, -kCardSize},
        {-hs, t + 0.001f, -kCardSize}
    };
    std::vector<glm::vec2> frontUVs = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };
    cardFrontQuad_ = std::make_unique<PolygonWithTexture>(frontVerts, frontUVs, glm::vec3(1.0f));

    // Back face sits at y = -0.001 so it is visible from behind without Z-fighting
    std::vector<glm::vec3> backVerts = {
        {-hs, -0.001f, -kCardSize},
        { hs, -0.001f, -kCardSize},
        { hs, -0.001f,  0.0f},
        {-hs, -0.001f,  0.0f}
    };
    std::vector<glm::vec2> backUVs = {
        {0.0f, 1.0f},
        {1.0f, 1.0f},
        {1.0f, 0.0f},
        {0.0f, 0.0f}
    };
    cardBackQuad_ = std::make_unique<PolygonWithTexture>(backVerts, backUVs, glm::vec3(1.0f));

    // Thin solid body built with Lecture 5's Cube primitive
    cardSlab_ = std::make_unique<Cube>(glm::vec3(0.0f), 1.0f, glm::vec3(0.84f, 0.72f, 0.42f));

    backTexture_ = texLoader.load("./textures/card_back.jpg", false,
                                  {0.16f, 0.24f, 0.42f}, {0.10f, 0.15f, 0.28f});
    cardBackQuad_->setTexture(backTexture_);

    const std::array<std::string, kInitialCards> paths = {
        "./textures/card_1.jpg",
        "./textures/card_2.jpg",
        "./textures/card_3.jpg",
        "./textures/card_4.jpg",
        "./textures/card_5.jpg"
    };
    const std::array<std::string, kInitialCards> names = {
        "Radwan",
        "Michel",
        "Antoun",
        "Jad",
        "Mr.Beast"
    };
    const std::array<glm::vec3, kInitialCards> fallbacks = {{
        {0.80f, 0.26f, 0.22f},
        {0.24f, 0.64f, 0.38f},
        {0.24f, 0.48f, 0.85f},
        {0.84f, 0.66f, 0.22f},
        {0.66f, 0.30f, 0.78f}
    }};

    cards_.clear();
    remainingIndices_.clear();
    playedCount_ = 0;

    for (int i = 0; i < kInitialCards; ++i) {
        CardItem item;
        item.id = i;
        item.characterName = names[i];
        item.frontTexture = texLoader.load(paths[i], false, fallbacks[i], fallbacks[i] * 0.55f);
        item.inDeck = true;
        item.startPos = {slotX(i), kTableSurfaceY, kDeckRowZ};
        item.targetPos = item.startPos;
        item.currentPos = item.startPos;

        cards_.push_back(item);
        remainingIndices_.push_back(i);
    }

    rng_.seed(std::random_device{}());
}

void CardDeck::drawRandomCard() {
    if (remainingIndices_.empty()) {
        // After 5 successful draws, additional G presses do nothing
        return;
    }

    std::uniform_int_distribution<size_t> dist(0, remainingIndices_.size() - 1);
    const size_t randomSlot = dist(rng_);
    const int cardIdx = remainingIndices_[randomSlot];
    remainingIndices_.erase(remainingIndices_.begin() + static_cast<std::ptrdiff_t>(randomSlot));

    CardItem& card = cards_[cardIdx];
    card.inDeck = false;
    card.isAnimating = true;
    card.moveProgress = 0.0f;
    card.startPos = card.currentPos;
    card.targetPos = {slotX(playedCount_), kTableSurfaceY, kPlayedRowZ};

    ++playedCount_;

    std::cout << "[CardDeck] Drew card #" << (card.id + 1) << " (" << card.characterName
              << ") -> Played slot " << playedCount_
              << " | Remaining in Deck: " << remainingIndices_.size() << "\n";
}

void CardDeck::update(float deltaTime, float standFactor) {
    const float targetDeg = standFactor * 90.0f;
    const float smoothRate = std::min(1.0f, deltaTime * 8.5f);
    currentTiltDeg_ += (targetDeg - currentTiltDeg_) * smoothRate;

    for (auto& card : cards_) {
        if (!card.isAnimating) continue;

        card.moveProgress = std::min(1.0f, card.moveProgress + deltaTime * 1.45f);
        const float t = card.moveProgress;
        // Cubic ease-in-out
        const float eased = (t < 0.5f)
            ? (4.0f * t * t * t)
            : (1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f);

        glm::vec3 pos = glm::mix(card.startPos, card.targetPos, eased);
        // Parabolic arc height so the card lifts gracefully over the tabletop
        pos.y += std::sin(t * kPi) * 0.44f;
        card.currentPos = pos;

        if (card.moveProgress >= 1.0f) {
            card.isAnimating = false;
            card.currentPos = card.targetPos;
        }
    }
}

void CardDeck::render(Shader& shader) const {
    shader.setBool("useVertexNormals", false);
    shader.setBool("isEmissive", false);
    shader.setFloat("shininess", 48.0f);
    shader.setVec3("tintColor", glm::vec3(1.0f));

    for (const auto& card : cards_) {
        const float scaleBoost = card.isAnimating
            ? (1.0f + 0.12f * std::sin(card.moveProgress * kPi))
            : 1.0f;

        // Lecture transformations: Translation -> Rotation (around X bottom pivot) -> Scaling
        glm::mat4 baseModel(1.0f);
        baseModel = glm::translate(baseModel, card.currentPos);
        baseModel = glm::rotate(baseModel, glm::radians(currentTiltDeg_), glm::vec3(1.0f, 0.0f, 0.0f));
        baseModel = glm::scale(baseModel, glm::vec3(scaleBoost));

        // 1. Solid card edge frame via Lecture 5 Cube
        glm::mat4 slabModel = glm::translate(baseModel, glm::vec3(0.0f, kCardThickness * 0.5f, -kCardSize * 0.5f));
        slabModel = glm::scale(slabModel, glm::vec3(kCardSize, kCardThickness, kCardSize));
        shader.setBool("useTexture", false);
        cardSlab_->transformation(slabModel);
        cardSlab_->draw(shader);

        // 2. Front character artwork via Lecture 5 PolygonWithTexture
        glActiveTexture(GL_TEXTURE0);
        shader.setInt("texture_diffuse1", 0);
        shader.setBool("useTexture", true);

        GLuint frontTex = card.frontTexture;
        cardFrontQuad_->setTexture(frontTex);
        cardFrontQuad_->transformation(baseModel);
        cardFrontQuad_->draw(shader);

        // 3. Back artwork via Lecture 5 PolygonWithTexture
        cardBackQuad_->transformation(baseModel);
        cardBackQuad_->draw(shader);
    }
}
