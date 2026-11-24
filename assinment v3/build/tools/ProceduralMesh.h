#pragma once
#ifndef PROCEDURAL_MESH_H
#define PROCEDURAL_MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <learnopengl/shader_m.h>
#include <vector>

// Small hand-built triangle mesh (no imported model): used for round objects that
// cannot be made from cubes, e.g. the teapot. Vertex layout matches CardGame.vs:
//   location 0 = position, 1 = normal, 2 = texcoord.
// Draw it with  shader.setBool("useVertexNormals", true)  for smooth shading.
class ProceduralMesh {
public:
    ProceduralMesh() = default;
    ~ProceduralMesh();
    ProceduralMesh(const ProceduralMesh&) = delete;
    ProceduralMesh& operator=(const ProceduralMesh&) = delete;
    ProceduralMesh(ProceduralMesh&& o) noexcept;
    ProceduralMesh& operator=(ProceduralMesh&& o) noexcept;

    // Surface of revolution around the Y axis. profile = (radius, y) points, listed
    // bottom -> top along the OUTSIDE of the shape (so normals face outward).
    static ProceduralMesh lathe(const std::vector<glm::vec2>& profile, int segments = 48);

    // Round tube swept along a path, with a per-point radius. Both ends are capped.
    static ProceduralMesh sweep(const std::vector<glm::vec3>& path,
                                const std::vector<float>& radii, int sides = 20);

    // Builds a mesh from explicit vertex arrays (pos / normal / uv) and triangle indices.
    static ProceduralMesh fromArrays(const std::vector<glm::vec3>& pos,
                                     const std::vector<glm::vec3>& normal,
                                     const std::vector<glm::vec2>& uv,
                                     const std::vector<unsigned int>& indices);

    // Samples a cubic Bezier curve into n points (n >= 2).
    static std::vector<glm::vec3> bezier(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2,
                                         glm::vec3 p3, int n);

    void draw(Shader& shader, const glm::mat4& model) const;

private:
    struct Vertex { glm::vec3 pos; glm::vec3 normal; glm::vec2 uv; };

    std::vector<Vertex> vertices_;
    std::vector<unsigned int> indices_;
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;

    void upload();
    void release();
};

#endif // PROCEDURAL_MESH_H
