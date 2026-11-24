#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <learnopengl/shader_m.h>
#include <learnopengl/model.h>

#include "tools/Cube.h"
#include "tools/Polygon.h"
#include "tools/ProceduralMesh.h"
#include "TextureLoader.h"

#include <memory>

class RoomEnvironment {
public:
    RoomEnvironment() = default;
    ~RoomEnvironment();

    void init(TextureLoader& texLoader);
    void render(Shader& shader, float elapsedSec) const;

private:
    static constexpr float kRoomW = 12.0f;
    static constexpr float kRoomH = 6.8f;
    static constexpr float kRoomD = 12.0f;

    std::unique_ptr<Cube> unitCube_;
    std::unique_ptr<PolygonWithTexture> floorQuad_;
    std::unique_ptr<PolygonWithTexture> wallZQuad_;
    std::unique_ptr<PolygonWithTexture> wallXQuad_;
    std::unique_ptr<PolygonWithTexture> tableTopQuad_;
    std::unique_ptr<PolygonWithTexture> tornPoster_;
    std::unique_ptr<Polygon> tornPosterBacking_;

    GLuint woodFloorTex_ = 0;
    GLuint wallpaperTex_ = 0;
    GLuint tableSurfaceTex_ = 0;
    GLuint posterTex_ = 0;
    GLuint monumentTex_ = 0;

    // Decorative Object #1: porcelain teapot (smooth procedural meshes: lathe body/lid + swept spout/handle)
    ProceduralMesh teapotBody_, teapotBand_, teapotLid_, teapotKnob_;
    ProceduralMesh teapotSpout_, teapotHandle_, teapotSaucer_;

    // Decorative Object #2: Damascene Sword monument (flat mosaic fin + concrete rim + two arched legs)
    ProceduralMesh monumentFront_, monumentBack_, monumentRim_, monumentLegL_, monumentLegR_;

    // Decorative Object #3 (the ONLY decorative object using a 3D model)
    std::unique_ptr<Model> decorModel_;
    glm::vec3 modelCenter_{0.0f};
    float modelMinY_ = 0.0f;
    float modelScaleToUnit_ = 1.0f;

    void drawBox(Shader& shader, const glm::vec3& pos, const glm::vec3& scale,
                 const glm::vec3& color, float shininess = 24.0f,
                 float rotYDeg = 0.0f, float rotZDeg = 0.0f) const;

    void drawRoomShell(Shader& shader) const;
    void drawGamingTable(Shader& shader) const;
    void drawZoneMat(Shader& shader, float centerZ,
                     const glm::vec3& fillCol, const glm::vec3& borderCol) const;
    void drawTornPoster(Shader& shader) const;

    // 3 Decorative Objects (2 procedural/primitive-built + 1 Assimp 3D model)
    void drawTeapot(Shader& shader, float elapsedSec) const;
    void drawMesh(Shader& shader, const ProceduralMesh& mesh, const glm::mat4& model,
                  const glm::vec3& color, float shininess) const;
    void drawDamasceneSword(Shader& shader, float elapsedSec) const;
    void drawTexturedMesh(Shader& shader, const ProceduralMesh& mesh, const glm::mat4& model,
                          GLuint tex, float shininess) const;
    void drawModelPedestal(Shader& shader, float elapsedSec) const;
};
