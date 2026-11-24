#pragma once

#include <glm/glm.hpp>

enum class CameraMode {
    TopView = 1,
    PlayerView = 2
};

class CameraSystem {
public:
    CameraSystem();

    void setMode(CameraMode newMode);
    CameraMode getMode() const { return mode_; }

    void update(float deltaTime);

    glm::mat4 getViewMatrix() const;
    glm::vec3 getPosition() const { return currentPos_; }

    // Smoothly transitions between 0.0 (Top View / flat cards) and 1.0 (Player View / standing cards)
    float getCardStandFactor() const;

private:
    CameraMode mode_ = CameraMode::TopView;
    float blend_ = 0.0f;

    glm::vec3 topPos_{0.0f, 5.25f, 0.05f};
    glm::vec3 topTarget_{0.0f, 1.00f, 0.00f};
    glm::vec3 topUp_{0.0f, 0.0f, -1.0f};

    glm::vec3 playerPos_{0.0f, 2.45f, 3.65f};
    glm::vec3 playerTarget_{0.0f, 1.38f, -0.50f};
    glm::vec3 playerUp_{0.0f, 1.0f, 0.0f};

    glm::vec3 currentPos_{};
    glm::vec3 currentTarget_{};
    glm::vec3 currentUp_{};
};
