#include "RoomEnvironment.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <vector>

RoomEnvironment::~RoomEnvironment() {
    if (floorQuad_) floorQuad_->deleteBuffers();
    if (wallZQuad_) wallZQuad_->deleteBuffers();
    if (wallXQuad_) wallXQuad_->deleteBuffers();
    if (tableTopQuad_) tableTopQuad_->deleteBuffers();
    if (tornPoster_) tornPoster_->deleteBuffers();
    if (tornPosterBacking_) tornPosterBacking_->deleteBuffers();
}

void RoomEnvironment::init(TextureLoader& texLoader) {
    // Reusable unit cube from Lecture 5 (center 0, edge 1, white color tinted via tintColor)
    unitCube_ = std::make_unique<Cube>(glm::vec3(0.0f), 1.0f, glm::vec3(1.0f));

    woodFloorTex_ = texLoader.load("./textures/floor_wood.jpg", true,
                                   {0.52f, 0.34f, 0.20f}, {0.42f, 0.26f, 0.14f});
    wallpaperTex_ = texLoader.load("./textures/wall_wallpaper.jpg", true,
                                   {0.28f, 0.35f, 0.30f}, {0.22f, 0.28f, 0.24f});
    tableSurfaceTex_ = texLoader.load("./textures/table_surface.jpg", true,
                                      {0.28f, 0.17f, 0.10f}, {0.20f, 0.12f, 0.07f});
    posterTex_ = texLoader.load("./textures/poster_art.jpg", false,
                                {0.20f, 0.32f, 0.56f}, {0.86f, 0.72f, 0.34f});
    monumentTex_ = texLoader.load("./textures/damascene_sword.jpg", false,
                                  {0.80f, 0.25f, 0.20f}, {0.15f, 0.30f, 0.65f});

    // Floor quad (XZ plane)
    {
        float hw = kRoomW * 0.5f, hd = kRoomD * 0.5f;
        floorQuad_ = std::make_unique<PolygonWithTexture>(
            std::vector<glm::vec3>{{-hw, 0.0f, hd}, {hw, 0.0f, hd}, {hw, 0.0f, -hd}, {-hw, 0.0f, -hd}},
            std::vector<glm::vec2>{{0.0f, 0.0f}, {4.0f, 0.0f}, {4.0f, 4.0f}, {0.0f, 4.0f}}
        );
        floorQuad_->setTexture(woodFloorTex_);
    }

    // Back/Front wallpaper quad (XY plane)
    {
        float hw = kRoomW * 0.5f, hh = kRoomH * 0.5f;
        wallZQuad_ = std::make_unique<PolygonWithTexture>(
            std::vector<glm::vec3>{{-hw, -hh, 0.0f}, {hw, -hh, 0.0f}, {hw, hh, 0.0f}, {-hw, hh, 0.0f}},
            std::vector<glm::vec2>{{0.0f, 0.0f}, {4.0f, 0.0f}, {4.0f, 2.0f}, {0.0f, 2.0f}}
        );
        wallZQuad_->setTexture(wallpaperTex_);
    }

    // Left/Right wallpaper quad (XY plane rotated +/-90 deg around Y)
    {
        float hw = kRoomD * 0.5f, hh = kRoomH * 0.5f;
        wallXQuad_ = std::make_unique<PolygonWithTexture>(
            std::vector<glm::vec3>{{-hw, -hh, 0.0f}, {hw, -hh, 0.0f}, {hw, hh, 0.0f}, {-hw, hh, 0.0f}},
            std::vector<glm::vec2>{{0.0f, 0.0f}, {4.0f, 0.0f}, {4.0f, 2.0f}, {0.0f, 2.0f}}
        );
        wallXQuad_->setTexture(wallpaperTex_);
    }

    // Tabletop surface textured quad
    {
        float tw = 2.20f, td = 1.60f;
        tableTopQuad_ = std::make_unique<PolygonWithTexture>(
            std::vector<glm::vec3>{{-tw, 1.001f, td}, {tw, 1.001f, td}, {tw, 1.001f, -td}, {-tw, 1.001f, -td}},
            std::vector<glm::vec2>{{0.0f, 0.0f}, {2.0f, 0.0f}, {2.0f, 2.0f}, {0.0f, 2.0f}}
        );
        tableTopQuad_->setTexture(tableSurfaceTex_);
    }

    // Artistic wall poster with visibly torn/missing top-right corner
    // Built as a GL_TRIANGLE_FAN using Lecture 5's PolygonWithTexture
    {
        const float pw = 1.75f;
        const float ph = 2.25f;
        const glm::vec2 fanCenter(0.36f, 0.40f);

        const std::vector<glm::vec2> perimeter = {
            {0.00f, 0.00f}, // bottom-left
            {1.00f, 0.00f}, // bottom-right
            {1.00f, 0.46f}, // tear starts on right edge
            {0.87f, 0.53f}, // jagged tooth 1
            {0.82f, 0.64f}, // jagged tooth 2
            {0.69f, 0.69f}, // jagged tooth 3
            {0.64f, 0.82f}, // jagged tooth 4
            {0.54f, 0.88f}, // jagged tooth 5
            {0.48f, 1.00f}, // tear exits top edge
            {0.00f, 1.00f}, // top-left
            {0.00f, 0.00f}  // close fan loop
        };

        std::vector<glm::vec3> pVerts;
        std::vector<glm::vec2> pUVs;
        pVerts.reserve(perimeter.size() + 1);
        pUVs.reserve(perimeter.size() + 1);

        pVerts.push_back({(fanCenter.x - 0.5f) * pw, (fanCenter.y - 0.5f) * ph, 0.0f});
        pUVs.push_back(fanCenter);

        for (const auto& uv : perimeter) {
            pVerts.push_back({(uv.x - 0.5f) * pw, (uv.y - 0.5f) * ph, 0.0f});
            pUVs.push_back(uv);
        }

        tornPoster_ = std::make_unique<PolygonWithTexture>(pVerts, pUVs, glm::vec3(1.0f));
        tornPoster_->setTexture(posterTex_);

        // Cream paper backing edge that makes the torn corner unmistakably visible
        tornPosterBacking_ = std::make_unique<Polygon>(pVerts, glm::vec3(0.90f, 0.87f, 0.80f));
    }

    // Load the SINGLE allowed 3D model decorative object via Lecture 5's Model class
    std::string modelPath = "./models/decor_model.glb";
    if (!std::filesystem::exists(modelPath)) {
        modelPath = "./models/alrihla.glb";
    }
    if (std::filesystem::exists(modelPath)) {
        decorModel_ = std::make_unique<Model>(modelPath);
        glm::vec3 minB(std::numeric_limits<float>::max());
        glm::vec3 maxB(std::numeric_limits<float>::lowest());
        bool hasVerts = false;
        for (const auto& mesh : decorModel_->meshes) {
            for (const auto& v : mesh.vertices) {
                minB = glm::min(minB, v.Position);
                maxB = glm::max(maxB, v.Position);
                hasVerts = true;
            }
        }
        if (hasVerts) {
            glm::vec3 extent = maxB - minB;
            float maxAxis = std::max({extent.x, extent.y, extent.z, 0.001f});
            modelCenter_ = (minB + maxB) * 0.5f;
            modelMinY_ = minB.y;
            modelScaleToUnit_ = 1.0f / maxAxis;
            std::cout << "[RoomEnvironment] Loaded 3D decor model: " << modelPath
                      << " (" << decorModel_->meshes.size() << " meshes)\n";
        }
    }

    // ---- Teapot meshes (local space: Y up, spout along +X, base at y = 0) ----
    using glm::vec2; using glm::vec3;
    teapotBody_ = ProceduralMesh::lathe({
        {0.00f, 0.000f}, {0.09f, 0.000f}, {0.13f, 0.008f}, {0.17f, 0.040f}, {0.20f, 0.090f},
        {0.215f, 0.150f}, {0.212f, 0.210f}, {0.190f, 0.270f}, {0.150f, 0.315f},
        {0.115f, 0.335f}, {0.108f, 0.345f}}, 56);
    teapotBand_ = ProceduralMesh::lathe({
        {0.2130f, 0.128f}, {0.2195f, 0.136f}, {0.2215f, 0.150f}, {0.2195f, 0.164f},
        {0.2155f, 0.172f}}, 56);
    teapotLid_ = ProceduralMesh::lathe({
        {0.112f, 0.338f}, {0.114f, 0.349f}, {0.095f, 0.368f}, {0.065f, 0.386f},
        {0.030f, 0.396f}, {0.000f, 0.399f}}, 48);
    teapotKnob_ = ProceduralMesh::lathe({
        {0.000f, 0.395f}, {0.022f, 0.397f}, {0.034f, 0.410f}, {0.030f, 0.426f},
        {0.014f, 0.436f}, {0.000f, 0.438f}}, 24);
    {
        auto path = ProceduralMesh::bezier(vec3(0.15f, 0.075f, 0.0f), vec3(0.30f, 0.070f, 0.0f),
                                           vec3(0.33f, 0.220f, 0.0f), vec3(0.43f, 0.300f, 0.0f), 16);
        std::vector<float> r;
        for (size_t i = 0; i < path.size(); ++i) {
            const float t = static_cast<float>(i) / (path.size() - 1);
            r.push_back(0.060f + (0.022f - 0.060f) * t + 0.006f * t * t * t);
        }
        teapotSpout_ = ProceduralMesh::sweep(path, r, 22);
    }
    {
        auto path = ProceduralMesh::bezier(vec3(-0.15f, 0.300f, 0.0f), vec3(-0.40f, 0.360f, 0.0f),
                                           vec3(-0.40f, 0.050f, 0.0f), vec3(-0.18f, 0.090f, 0.0f), 18);
        std::vector<float> r(path.size(), 0.021f);
        teapotHandle_ = ProceduralMesh::sweep(path, r, 18);
    }
    teapotSaucer_ = ProceduralMesh::lathe({
        {0.000f, 0.000f}, {0.220f, 0.000f}, {0.310f, 0.010f}, {0.345f, 0.032f},
        {0.338f, 0.036f}, {0.300f, 0.024f}, {0.220f, 0.016f}, {0.000f, 0.016f}}, 56);

    // ---- Damascene Sword monument ----
    // Outline traced from the reference photo (pixel coords). The fin is symmetric about x = 383:
    // hw(y) = half-width in px at image row y. Photo crop used as texture: x 150..620, y 40..860.
    {
        struct Row { float y, hw; };
        static const Row table[] = {
            {840, 4}, {820, 18}, {800, 36}, {775, 50}, {750, 64}, {725, 92}, {700, 122}, {680, 155},
            {662, 190}, {650, 208}, {644, 213}, {636, 205}, {625, 172}, {612, 148}, {600, 128},
            {575, 100}, {550, 72}, {525, 58}, {500, 46}, {450, 38.5f}, {400, 34.5f}, {300, 26.5f},
            {200, 19}, {100, 11.5f}, {75, 9}, {50, 2}};
        const int nTable = static_cast<int>(sizeof(table) / sizeof(table[0]));
        auto hwAt = [&](float y) {
            for (int i = 0; i + 1 < nTable; ++i) {
                const Row& a = table[i]; const Row& b = table[i + 1];
                if (y <= a.y && y >= b.y) {
                    const float t = (a.y - y) / (a.y - b.y);
                    return a.hw + (b.hw - a.hw) * t;
                }
            }
            return table[nTable - 1].hw;
        };

        // the blade leans slightly: its centre line is x = 379 at the tip and x = 384 near the wings
        auto axisAt = [](float y) {
            if (y <= 450.0f) return 379.0f;
            if (y >= 500.0f) return 384.0f;
            return 379.0f + 5.0f * (y - 450.0f) / 50.0f;
        };
        const float s = 3.0f / 830.0f;            // metres per photo pixel (monument is ~3 m tall)
        const int kCols = 28;
        const float yBottom = 840.0f, yTop = 50.0f, yStep = 5.0f;
        std::vector<float> ys;
        for (float y = yBottom; y >= yTop; y -= yStep) ys.push_back(y);
        const int rows = static_cast<int>(ys.size());

        auto buildFace = [&](bool front) {
            std::vector<glm::vec3> P, N; std::vector<glm::vec2> UV; std::vector<unsigned> I;
            for (int j = 0; j < rows; ++j) {
                const float hw = hwAt(ys[j]);
                const float hz = 0.014f + 0.00017f * hw;
                for (int c = 0; c <= kCols; ++c) {
                    const float t = -1.0f + 2.0f * c / kCols;
                    const float dx = t * hw;                       // px offset from the axis
                    P.push_back({(axisAt(ys[j]) - 383.0f + dx) * s, (880.0f - ys[j]) * s, front ? hz : -hz});
                    N.push_back({0.0f, 0.0f, front ? 1.0f : -1.0f});
                    const float px = front ? (axisAt(ys[j]) + dx) : (axisAt(ys[j]) - dx);   // back face is un-mirrored
                    UV.push_back({(px - 150.0f) / 470.0f, 1.0f - (ys[j] - 40.0f) / 820.0f});
                }
            }
            const int stride = kCols + 1;
            for (int j = 0; j + 1 < rows; ++j)
                for (int c = 0; c < kCols; ++c) {
                    const unsigned a = j * stride + c, b = a + 1, cc = a + stride, d = cc + 1;
                    I.insert(I.end(), {a, b, d, a, d, cc});
                }
            return ProceduralMesh::fromArrays(P, N, UV, I);
        };
        monumentFront_ = buildFace(true);
        monumentBack_  = buildFace(false);

        // Concrete rim: thin side walls joining the front and back faces along both edges
        {
            std::vector<glm::vec3> P, N; std::vector<glm::vec2> UV; std::vector<unsigned> I;
            for (int side : {-1, 1}) {
                const unsigned base = static_cast<unsigned>(P.size());
                for (int j = 0; j < rows; ++j) {
                    const float hw = hwAt(ys[j]);
                    const float hz = 0.014f + 0.00017f * hw;
                    const float yPrev = ys[std::max(j - 1, 0)], yNext = ys[std::min(j + 1, rows - 1)];
                    const float tx = side * (hwAt(yNext) - hwAt(yPrev)) * s;       // d x
                    const float ty = (yPrev - yNext) * s;                           // d y (up)
                    // outward normal of the edge curve: perpendicular to tangent (tx, ty) pointing away from axis
                    glm::vec2 nn(ty, -tx);
                    if (side < 0) nn = -nn;
                    if (glm::length(nn) < 1e-8f) nn = glm::vec2(static_cast<float>(side), 0.0f);
                    nn = glm::normalize(nn);
                    if (nn.x * side < 0.0f) nn = -nn;
                    const glm::vec3 nrm(nn.x, nn.y, 0.0f);
                    const float x = (axisAt(ys[j]) - 383.0f + side * hw) * s, y = (880.0f - ys[j]) * s;
                    P.push_back({x, y,  hz}); N.push_back(nrm); UV.push_back({0.0f, 0.0f});
                    P.push_back({x, y, -hz}); N.push_back(nrm); UV.push_back({1.0f, 0.0f});
                }
                for (int j = 0; j + 1 < rows; ++j) {
                    const unsigned a = base + 2 * j, b = a + 1, c = a + 2, d = a + 3;
                    I.insert(I.end(), {a, b, d, a, d, c});
                }
            }
            monumentRim_ = ProceduralMesh::fromArrays(P, N, UV, I);
        }

        // Two concrete arch legs (left/right), swept along a curve from the wing tips down to the base
        auto toWorld = [&](float px, float py, float sign) { return glm::vec3(sign * (px - 383.0f) * s, (880.0f - py) * s, 0.0f); };
        for (int side : {-1, 1}) {
            auto path = ProceduralMesh::bezier(toWorld(185, 668, 1), toWorld(280, 730, 1),
                                               toWorld(335, 790, 1), toWorld(352, 880, 1), 20);
            if (side > 0) for (auto& p : path) p.x = -p.x;           // toWorld gives the left (-x) leg; mirror it for the right
            std::vector<float> r;
            for (size_t i = 0; i < path.size(); ++i) {
                const float t = static_cast<float>(i) / (path.size() - 1);
                r.push_back(0.032f + 0.030f * std::pow(t, 0.8f));
            }
            (side < 0 ? monumentLegL_ : monumentLegR_) = ProceduralMesh::sweep(path, r, 20);
        }
    }
}

void RoomEnvironment::drawBox(Shader& shader, const glm::vec3& pos, const glm::vec3& scale,
                              const glm::vec3& color, float shininess,
                              float rotYDeg, float rotZDeg) const {
    glm::mat4 m(1.0f);
    m = glm::translate(m, pos);
    if (rotYDeg != 0.0f) m = glm::rotate(m, glm::radians(rotYDeg), glm::vec3(0.0f, 1.0f, 0.0f));
    if (rotZDeg != 0.0f) m = glm::rotate(m, glm::radians(rotZDeg), glm::vec3(0.0f, 0.0f, 1.0f));
    m = glm::scale(m, scale);

    shader.setBool("useTexture", false);
    shader.setBool("useVertexNormals", false);
    shader.setBool("isEmissive", false);
    shader.setVec3("tintColor", color);
    shader.setFloat("shininess", shininess);

    unitCube_->transformation(m);
    unitCube_->draw(shader);
}

void RoomEnvironment::render(Shader& shader, float elapsedSec) const {
    drawRoomShell(shader);
    drawGamingTable(shader);
    drawTornPoster(shader);

    // Three distinct decorative objects (2 primitives + 1 Assimp 3D model)
    drawTeapot(shader, elapsedSec);
    drawDamasceneSword(shader, elapsedSec);
    drawModelPedestal(shader, elapsedSec);

    shader.setVec3("tintColor", glm::vec3(1.0f));
}

void RoomEnvironment::drawRoomShell(Shader& shader) const {
    glActiveTexture(GL_TEXTURE0);
    shader.setInt("texture_diffuse1", 0);
    shader.setVec3("tintColor", glm::vec3(1.0f));
    shader.setBool("useVertexNormals", false);
    shader.setBool("isEmissive", false);
    shader.setFloat("shininess", 16.0f);

    // 1. Wooden Floor
    shader.setBool("useTexture", true);
    floorQuad_->transformation(glm::mat4(1.0f));
    floorQuad_->draw(shader);

    // 2. Ceiling
    drawBox(shader, {0.0f, kRoomH, 0.0f}, {kRoomW, 0.08f, kRoomD}, {0.86f, 0.83f, 0.76f}, 8.0f);

    // 3. Four Wallpaper Walls
    shader.setVec3("tintColor", glm::vec3(1.0f));
    shader.setBool("useTexture", true);
    shader.setFloat("shininess", 12.0f);

    // Back wall (-Z)
    wallZQuad_->transformation(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, kRoomH * 0.5f, -kRoomD * 0.5f)));
    wallZQuad_->draw(shader);

    // Front wall (+Z)
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, kRoomH * 0.5f, kRoomD * 0.5f));
        m = glm::rotate(m, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        wallZQuad_->transformation(m);
        wallZQuad_->draw(shader);
    }

    // Left wall (-X)
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(-kRoomW * 0.5f, kRoomH * 0.5f, 0.0f));
        m = glm::rotate(m, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        wallXQuad_->transformation(m);
        wallXQuad_->draw(shader);
    }

    // Right wall (+X)
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(kRoomW * 0.5f, kRoomH * 0.5f, 0.0f));
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        wallXQuad_->transformation(m);
        wallXQuad_->draw(shader);
    }

    // Wooden Baseboards along walls
    const glm::vec3 trimCol{0.24f, 0.15f, 0.09f};
    drawBox(shader, {0.0f, 0.09f, -kRoomD * 0.5f + 0.03f}, {kRoomW, 0.18f, 0.06f}, trimCol);
    drawBox(shader, {0.0f, 0.09f,  kRoomD * 0.5f - 0.03f}, {kRoomW, 0.18f, 0.06f}, trimCol);
    drawBox(shader, {-kRoomW * 0.5f + 0.03f, 0.09f, 0.0f}, {0.06f, 0.18f, kRoomD}, trimCol);
    drawBox(shader, { kRoomW * 0.5f - 0.03f, 0.09f, 0.0f}, {0.06f, 0.18f, kRoomD}, trimCol);

    // Overhead Pendant Lamp Fixture (mounted near ceiling, well above Top View camera at y = 5.25)
    drawBox(shader, {0.0f, 6.50f, 0.0f}, {0.06f, 0.60f, 0.06f}, {0.16f, 0.16f, 0.18f});
    drawBox(shader, {0.0f, 6.18f, 0.0f}, {0.72f, 0.14f, 0.72f}, {0.20f, 0.28f, 0.22f});

    // Emissive Bulb
    shader.setBool("useTexture", false);
    shader.setBool("isEmissive", true);
    shader.setVec3("tintColor", glm::vec3(1.0f, 0.95f, 0.80f));
    glm::mat4 bulbM = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 6.06f, 0.0f));
    bulbM = glm::scale(bulbM, glm::vec3(0.22f));
    unitCube_->transformation(bulbM);
    unitCube_->draw(shader);
    shader.setBool("isEmissive", false);
}

void RoomEnvironment::drawGamingTable(Shader& shader) const {
    // Tabletop body + textured wooden top surface
    drawBox(shader, {0.0f, 0.94f, 0.0f}, {4.40f, 0.12f, 3.20f}, {0.26f, 0.16f, 0.10f});

    glActiveTexture(GL_TEXTURE0);
    shader.setInt("texture_diffuse1", 0);
    shader.setVec3("tintColor", glm::vec3(1.0f));
    shader.setBool("useTexture", true);
    shader.setFloat("shininess", 28.0f);
    tableTopQuad_->transformation(glm::mat4(1.0f));
    tableTopQuad_->draw(shader);

    // Outer table trim rails
    const glm::vec3 rail{0.20f, 0.12f, 0.07f};
    drawBox(shader, {0.0f, 0.96f, -1.62f}, {4.52f, 0.16f, 0.08f}, rail);
    drawBox(shader, {0.0f, 0.96f,  1.62f}, {4.52f, 0.16f, 0.08f}, rail);
    drawBox(shader, {-2.22f, 0.96f, 0.0f}, {0.08f, 0.16f, 3.32f}, rail);
    drawBox(shader, { 2.22f, 0.96f, 0.0f}, {0.08f, 0.16f, 3.32f}, rail);

    // 4 Table Legs
    drawBox(shader, {-1.92f, 0.44f, -1.32f}, {0.16f, 0.88f, 0.16f}, rail);
    drawBox(shader, { 1.92f, 0.44f, -1.32f}, {0.16f, 0.88f, 0.16f}, rail);
    drawBox(shader, {-1.92f, 0.44f,  1.32f}, {0.16f, 0.88f, 0.16f}, rail);
    drawBox(shader, { 1.92f, 0.44f,  1.32f}, {0.16f, 0.88f, 0.16f}, rail);

    // 3 Distinct Board Sections:
    // 1. Opponent Cards (Empty)
    drawZoneMat(shader, -1.04f, {0.36f, 0.14f, 0.14f}, {0.74f, 0.30f, 0.28f});
    // 2. Player Played Cards
    drawZoneMat(shader, -0.06f, {0.14f, 0.31f, 0.21f}, {0.82f, 0.68f, 0.32f});
    // 3. Player Deck
    drawZoneMat(shader,  0.98f, {0.14f, 0.22f, 0.36f}, {0.42f, 0.68f, 0.92f});
}

void RoomEnvironment::drawZoneMat(Shader& shader, float centerZ,
                                  const glm::vec3& fillCol, const glm::vec3& borderCol) const {
    drawBox(shader, {0.0f, 1.003f, centerZ}, {3.86f, 0.004f, 0.78f}, borderCol, 12.0f);
    drawBox(shader, {0.0f, 1.005f, centerZ}, {3.78f, 0.004f, 0.70f}, fillCol, 8.0f);

    for (int i = 0; i < 5; ++i) {
        float sx = (i - 2) * 0.68f;
        drawBox(shader, {sx, 1.007f, centerZ}, {0.58f, 0.002f, 0.58f}, borderCol * 0.65f, 10.0f);
        drawBox(shader, {sx, 1.008f, centerZ}, {0.54f, 0.002f, 0.54f}, fillCol * 0.90f, 8.0f);
    }
}

void RoomEnvironment::drawTornPoster(Shader& shader) const {
    shader.setBool("useVertexNormals", false);
    shader.setBool("isEmissive", false);
    shader.setVec3("tintColor", glm::vec3(1.0f));

    // Cream paper tear edge slightly behind poster
    {
        glm::mat4 backM(1.0f);
        backM = glm::translate(backM, glm::vec3(-0.35f, 2.45f, -kRoomD * 0.5f + 0.022f));
        backM = glm::scale(backM, glm::vec3(1.025f, 1.018f, 1.0f));
        shader.setBool("useTexture", false);
        shader.setFloat("shininess", 8.0f);
        tornPosterBacking_->transformation(backM);
        tornPosterBacking_->draw(shader);
    }

    // Torn artwork poster mounted on back wall (centered at y = 2.45 so full poster + torn corner show in Camera 2)
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(-0.35f, 2.45f, -kRoomD * 0.5f + 0.036f));
        glActiveTexture(GL_TEXTURE0);
        shader.setInt("texture_diffuse1", 0);
        shader.setBool("useTexture", true);
        shader.setFloat("shininess", 16.0f);
        tornPoster_->transformation(m);
        tornPoster_->draw(shader);
    }
}

void RoomEnvironment::drawMesh(Shader& shader, const ProceduralMesh& mesh, const glm::mat4& model,
                               const glm::vec3& color, float shininess) const {
    shader.setBool("useTexture", false);
    shader.setBool("useVertexNormals", true);   // smooth, rounded shading
    shader.setBool("isEmissive", false);
    shader.setVec3("objectColor", glm::vec3(1.0f));
    shader.setVec3("tintColor", color);
    shader.setFloat("shininess", shininess);
    mesh.draw(shader, model);
}

void RoomEnvironment::drawTeapot(Shader& shader, float elapsedSec) const {
    const glm::vec3 tableSpot{-2.00f, 1.00f, -1.15f};
    const float kScale = 0.90f;
    const glm::vec3 glaze{0.10f, 0.26f, 0.62f};       // cobalt-blue porcelain
    const glm::vec3 gold{0.90f, 0.72f, 0.26f};
    const glm::vec3 cream{0.93f, 0.89f, 0.78f};

    // Saucer sits flat on the table; teapot rests on top, turned so the spout points toward the player.
    glm::mat4 saucerM = glm::translate(glm::mat4(1.0f), tableSpot);
    saucerM = glm::scale(saucerM, glm::vec3(kScale));
    drawMesh(shader, teapotSaucer_, saucerM, cream, 70.0f);

    glm::mat4 pot = glm::translate(glm::mat4(1.0f), tableSpot + glm::vec3(0.0f, 0.016f * kScale, 0.0f));
    pot = glm::rotate(pot, glm::radians(-70.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    pot = glm::scale(pot, glm::vec3(kScale));

    drawMesh(shader, teapotBody_,   pot, glaze, 110.0f);
    drawMesh(shader, teapotSpout_,  pot, glaze, 110.0f);
    drawMesh(shader, teapotHandle_, pot, glaze, 110.0f);
    drawMesh(shader, teapotLid_,    pot, glaze, 110.0f);
    drawMesh(shader, teapotBand_,   pot, gold, 120.0f);
    drawMesh(shader, teapotKnob_,   pot, gold, 120.0f);

    // Three soft steam puffs drifting up from the spout tip (looping animation)
    const glm::vec3 tip = glm::vec3(pot * glm::vec4(0.43f, 0.31f, 0.0f, 1.0f));
    for (int i = 0; i < 3; ++i) {
        float phase = elapsedSec * 0.32f + i / 3.0f;
        phase -= std::floor(phase);                          // 0..1
        const float size = 0.075f * (0.55f + 0.9f * phase) * (1.0f - phase);
        if (size < 0.004f) continue;
        const glm::vec3 pos = tip + glm::vec3(0.07f * std::sin(phase * 6.0f + i * 2.1f) * phase,
                                              0.04f + 0.50f * phase,
                                              0.07f * std::cos(phase * 5.0f + i) * phase);
        const float shade = 0.97f - 0.20f * phase;
        drawBox(shader, pos, glm::vec3(size), glm::vec3(shade), 4.0f, phase * 260.0f + i * 40.0f);
    }
}

void RoomEnvironment::drawTexturedMesh(Shader& shader, const ProceduralMesh& mesh, const glm::mat4& model,
                                       GLuint tex, float shininess) const {
    glActiveTexture(GL_TEXTURE0);
    shader.setInt("texture_diffuse1", 0);
    glBindTexture(GL_TEXTURE_2D, tex);
    shader.setBool("useTexture", true);
    shader.setBool("useVertexNormals", true);
    shader.setBool("isEmissive", false);
    shader.setVec3("objectColor", glm::vec3(1.0f));
    shader.setVec3("tintColor", glm::vec3(1.0f));
    shader.setFloat("shininess", shininess);
    mesh.draw(shader, model);
}

void RoomEnvironment::drawDamasceneSword(Shader& shader, float /*elapsedSec*/) const {
    const glm::vec3 spot{3.85f, 0.0f, -4.45f};
    const glm::vec3 concrete{0.80f, 0.79f, 0.76f};
    const glm::vec3 stone{0.52f, 0.50f, 0.47f};

    // Two-step stone plinth (primitive cubes)
    drawBox(shader, spot + glm::vec3(0.0f, 0.04f, 0.0f), {2.10f, 0.08f, 1.10f}, stone * 0.85f, 10.0f, -15.0f);
    drawBox(shader, spot + glm::vec3(0.0f, 0.10f, 0.0f), {1.75f, 0.04f, 0.85f}, stone, 14.0f, -15.0f);

    // The monument itself: turned slightly toward the middle of the room
    glm::mat4 m = glm::translate(glm::mat4(1.0f), spot + glm::vec3(0.0f, 0.12f, 0.0f));
    m = glm::rotate(m, glm::radians(-15.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    drawTexturedMesh(shader, monumentFront_, m, monumentTex_, 24.0f);
    drawTexturedMesh(shader, monumentBack_,  m, monumentTex_, 24.0f);
    drawMesh(shader, monumentRim_,  m, concrete, 14.0f);
    drawMesh(shader, monumentLegL_, m, concrete, 14.0f);
    drawMesh(shader, monumentLegR_, m, concrete, 14.0f);
}

void RoomEnvironment::drawModelPedestal(Shader& shader, float elapsedSec) const {
    const glm::vec3 base{-3.85f, 0.0f, -4.15f};
    drawBox(shader, base + glm::vec3(0.0f, 0.08f, 0.0f), {1.06f, 0.16f, 1.06f}, {0.26f, 0.24f, 0.22f});
    drawBox(shader, base + glm::vec3(0.0f, 0.72f, 0.0f), {0.72f, 1.12f, 0.72f}, {0.82f, 0.80f, 0.76f}, 48.0f);
    drawBox(shader, base + glm::vec3(0.0f, 1.32f, 0.0f), {0.98f, 0.08f, 0.98f}, {0.26f, 0.24f, 0.22f});

    if (decorModel_) {
        glm::mat4 world(1.0f);
        world = glm::translate(world, base + glm::vec3(0.0f, 1.38f, 0.0f));
        world = glm::rotate(world, elapsedSec * 0.65f, glm::vec3(0.0f, 1.0f, 0.0f));
        world = glm::scale(world, glm::vec3(1.15f * modelScaleToUnit_));
        world = glm::translate(world, glm::vec3(-modelCenter_.x, -modelMinY_, -modelCenter_.z));

        shader.setMat4("model", world);
        shader.setVec3("objectColor", glm::vec3(1.0f));
        shader.setVec3("tintColor", glm::vec3(1.0f));
        shader.setBool("useTexture", true);
        shader.setBool("useVertexNormals", true);
        shader.setBool("isEmissive", false);
        shader.setFloat("shininess", 48.0f);
        decorModel_->Draw(shader);
    } else {
        drawBox(shader, base + glm::vec3(0.0f, 1.85f, 0.0f), {0.55f, 0.95f, 0.55f}, {0.78f, 0.60f, 0.24f}, 96.0f);
    }
}
