#include "libs/glad_loader_docs.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <learnopengl/shader_m.h>

#include "cardgame/CameraSystem.h"
#include "cardgame/CardDeck.h"
#include "cardgame/DecorScoring.h"
#include "cardgame/RoomEnvironment.h"
#include "cardgame/TextureLoader.h"

#include <algorithm>
#include <iostream>
#include <vector>

struct AppContext {
    CameraSystem camera;
    CardDeck deck;
    int width = 2560;
    int height = 1440;
};

static void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (ctx && width > 0 && height > 0) {
        ctx->width = width;
        ctx->height = height;
    }
}

static void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    // Edge-triggered key press prevents multiple card draws from holding down G
    if (action != GLFW_PRESS) return;

    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (!ctx) return;

    switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_1:
            ctx->camera.setMode(CameraMode::TopView);
            break;
        case GLFW_KEY_2:
            ctx->camera.setMode(CameraMode::PlayerView);
            break;
        case GLFW_KEY_G:
            ctx->deck.drawRandomCard();
            break;
        default:
            break;
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "[GLFW] Failed to initialize.\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    AppContext app;
    GLFWwindow* window = glfwCreateWindow(app.width, app.height,
                                          "3D Card Game - OpenGL Assignment (HW-2026)",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "[GLFW] Failed to create window.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, &app);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[GLAD] Failed to initialize OpenGL function pointers.\n";
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    Shader sceneShader("./shaders/vs/CardGame.vs", "./shaders/fs/CardGame.fs");

    TextureLoader textureLoader;
    RoomEnvironment room;
    room.init(textureLoader);
    app.deck.init(textureLoader);

    // Decorative Object IDF Originality Report (Note 3 in HW-2026.pdf)
    std::vector<DecorItemRecord> decorItems = {
        {"Porcelain Teapot with Saucer", false, 2},
        {"Damascene Sword Monument",    false, 2},
        {"Pedestal Trophy (3D Model)",  true,  2}
    };
    DecorScoring::printOriginalityReport(/*totalTeams=*/50, decorItems, /*maxDecorPoints=*/2.0);

    std::cout << "================ Controls ================\n"
              << "  [1] Camera 1: Top-Down View (cards horizontal)\n"
              << "  [2] Camera 2: Player View (cards stand vertically)\n"
              << "  [G] Draw 1 random card from Deck -> Played area (max 5)\n"
              << "  [ESC] Exit\n"
              << "==========================================\n\n";

    float lastFrame = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        const float currentFrame = static_cast<float>(glfwGetTime());
        const float deltaTime = std::min(0.05f, currentFrame - lastFrame);
        lastFrame = currentFrame;

        app.camera.update(deltaTime);
        app.deck.update(deltaTime, app.camera.getCardStandFactor());

        glClearColor(0.06f, 0.06f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sceneShader.use();

        const float aspect = static_cast<float>(app.width) / static_cast<float>(std::max(1, app.height));
        const glm::mat4 projection = glm::perspective(glm::radians(46.0f), aspect, 0.1f, 60.0f);
        const glm::mat4 view = app.camera.getViewMatrix();

        sceneShader.setMat4("projection", projection);
        sceneShader.setMat4("view", view);
        sceneShader.setVec3("viewPos", app.camera.getPosition());
        sceneShader.setInt("texture_diffuse1", 0);

        // Two warm indoor point lights (overhead pendant lamp + player-side fill light)
        sceneShader.setInt("numLights", 2);
        sceneShader.setVec3("lights[0].position", glm::vec3(0.0f, 5.75f, 0.0f));
        sceneShader.setVec3("lights[0].color", glm::vec3(1.0f, 0.94f, 0.82f));
        sceneShader.setFloat("lights[0].intensity", 2.05f);

        sceneShader.setVec3("lights[1].position", glm::vec3(1.8f, 3.30f, 3.6f));
        sceneShader.setVec3("lights[1].color", glm::vec3(0.84f, 0.89f, 1.0f));
        sceneShader.setFloat("lights[1].intensity", 0.90f);

        room.render(sceneShader, currentFrame);
        app.deck.render(sceneShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
