#include "CameraSystem.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>

CameraSystem::CameraSystem()
    : currentPos_(topPos_), currentTarget_(topTarget_), currentUp_(topUp_) {}

void CameraSystem::setMode(CameraMode newMode) {
    if (mode_ == newMode) return;
    mode_ = newMode;
    std::cout << "[Camera] Active Mode: "
              << (mode_ == CameraMode::TopView
                      ? "1 - Top-Down Table View (cards horizontal)"
                      : "2 - Player Perspective View (cards vertical)")
              << "\n";
}

void CameraSystem::update(float deltaTime) {
    const float targetBlend = (mode_ == CameraMode::PlayerView) ? 1.0f : 0.0f;
    const float speed = 3.0f;

    if (blend_ < targetBlend) {
        blend_ = std::min(targetBlend, blend_ + speed * deltaTime);
    } else if (blend_ > targetBlend) {
        blend_ = std::max(targetBlend, blend_ - speed * deltaTime);
    }

    const float s = getCardStandFactor();
    currentPos_ = glm::mix(topPos_, playerPos_, s);
    currentTarget_ = glm::mix(topTarget_, playerTarget_, s);
    currentUp_ = glm::normalize(glm::mix(topUp_, playerUp_, s));
}

glm::mat4 CameraSystem::getViewMatrix() const {
    return glm::lookAt(currentPos_, currentTarget_, currentUp_);
}

float CameraSystem::getCardStandFactor() const {
    // Cubic smoothstep easing: 3t^2 - 2t^3
    return blend_ * blend_ * (3.0f - 2.0f * blend_);
}
