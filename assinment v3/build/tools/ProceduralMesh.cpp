#include "ProceduralMesh.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

ProceduralMesh::~ProceduralMesh() { release(); }

ProceduralMesh::ProceduralMesh(ProceduralMesh&& o) noexcept
    : vertices_(std::move(o.vertices_)), indices_(std::move(o.indices_)),
      vao_(o.vao_), vbo_(o.vbo_), ebo_(o.ebo_) {
    o.vao_ = o.vbo_ = o.ebo_ = 0;
}

ProceduralMesh& ProceduralMesh::operator=(ProceduralMesh&& o) noexcept {
    if (this != &o) {
        release();
        vertices_ = std::move(o.vertices_);
        indices_ = std::move(o.indices_);
        vao_ = o.vao_; vbo_ = o.vbo_; ebo_ = o.ebo_;
        o.vao_ = o.vbo_ = o.ebo_ = 0;
    }
    return *this;
}

void ProceduralMesh::release() {
    // The window/context may already be destroyed when static-lifetime owners clean up.
    if (!glfwGetCurrentContext()) { vao_ = vbo_ = ebo_ = 0; return; }
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    vao_ = vbo_ = ebo_ = 0;
}

void ProceduralMesh::upload() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), vertices_.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int), indices_.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glBindVertexArray(0);
}

void ProceduralMesh::draw(Shader& shader, const glm::mat4& model) const {
    if (!vao_) return;
    shader.setMat4("model", model);
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

ProceduralMesh ProceduralMesh::lathe(const std::vector<glm::vec2>& profile, int segments) {
    ProceduralMesh m;
    const int n = static_cast<int>(profile.size());
    if (n < 2 || segments < 3) return m;

    // Outward normal of each profile point in the (radius, y) plane = (dy, -dr), averaged.
    std::vector<glm::vec2> nrm(n);
    for (int i = 0; i < n; ++i) {
        const glm::vec2 a = profile[std::max(i - 1, 0)];
        const glm::vec2 b = profile[std::min(i + 1, n - 1)];
        const glm::vec2 t = b - a;
        glm::vec2 v(t.y, -t.x);
        const float len = std::sqrt(v.x * v.x + v.y * v.y);
        nrm[i] = (len > 1e-8f) ? v / len : glm::vec2(1.0f, 0.0f);
    }

    for (int i = 0; i < n; ++i) {
        for (int s = 0; s <= segments; ++s) {
            const float u = static_cast<float>(s) / segments;
            const float ang = u * 2.0f * kPi;
            const float c = std::cos(ang), sn = std::sin(ang);
            Vertex v;
            v.pos = glm::vec3(profile[i].x * c, profile[i].y, profile[i].x * sn);
            v.normal = glm::normalize(glm::vec3(nrm[i].x * c, nrm[i].y, nrm[i].x * sn));
            v.uv = glm::vec2(u, static_cast<float>(i) / (n - 1));
            m.vertices_.push_back(v);
        }
    }
    const int stride = segments + 1;
    for (int i = 0; i < n - 1; ++i) {
        for (int s = 0; s < segments; ++s) {
            const unsigned a = i * stride + s, b = a + 1, c = a + stride, d = c + 1;
            m.indices_.insert(m.indices_.end(), {a, c, b, b, c, d});
        }
    }
    m.upload();
    return m;
}

ProceduralMesh ProceduralMesh::fromArrays(const std::vector<glm::vec3>& pos,
                                          const std::vector<glm::vec3>& normal,
                                          const std::vector<glm::vec2>& uv,
                                          const std::vector<unsigned int>& indices) {
    ProceduralMesh m;
    if (pos.empty() || pos.size() != normal.size() || pos.size() != uv.size()) return m;
    for (size_t i = 0; i < pos.size(); ++i) m.vertices_.push_back({pos[i], normal[i], uv[i]});
    m.indices_ = indices;
    m.upload();
    return m;
}

std::vector<glm::vec3> ProceduralMesh::bezier(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2,
                                              glm::vec3 p3, int n) {
    std::vector<glm::vec3> pts;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / (n - 1), u = 1.0f - t;
        pts.push_back(u * u * u * p0 + 3.0f * u * u * t * p1 + 3.0f * u * t * t * p2 + t * t * t * p3);
    }
    return pts;
}

ProceduralMesh ProceduralMesh::sweep(const std::vector<glm::vec3>& path,
                                     const std::vector<float>& radii, int sides) {
    ProceduralMesh m;
    const int n = static_cast<int>(path.size());
    if (n < 2 || static_cast<int>(radii.size()) != n || sides < 3) return m;

    // Parallel-transport frame along the path (no twisting).
    std::vector<glm::vec3> T(n), N(n), B(n);
    for (int i = 0; i < n; ++i)
        T[i] = glm::normalize(path[std::min(i + 1, n - 1)] - path[std::max(i - 1, 0)]);
    glm::vec3 ref = (std::fabs(T[0].y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    N[0] = glm::normalize(glm::cross(ref, T[0]));
    B[0] = glm::cross(T[0], N[0]);
    for (int i = 1; i < n; ++i) {
        glm::vec3 nn = N[i - 1] - T[i] * glm::dot(N[i - 1], T[i]);
        N[i] = glm::normalize(nn);
        B[i] = glm::cross(T[i], N[i]);
    }

    for (int i = 0; i < n; ++i) {
        for (int s = 0; s <= sides; ++s) {
            const float u = static_cast<float>(s) / sides;
            const float a = u * 2.0f * kPi;
            const glm::vec3 dir = std::cos(a) * N[i] + std::sin(a) * B[i];
            Vertex v;
            v.pos = path[i] + dir * radii[i];
            v.normal = dir;
            v.uv = glm::vec2(u, static_cast<float>(i) / (n - 1));
            m.vertices_.push_back(v);
        }
    }
    const int stride = sides + 1;
    for (int i = 0; i < n - 1; ++i) {
        for (int s = 0; s < sides; ++s) {
            const unsigned a = i * stride + s, b = a + 1, c = a + stride, d = c + 1;
            m.indices_.insert(m.indices_.end(), {a, c, b, b, c, d});
        }
    }

    // End caps (flat discs)
    auto cap = [&](int ring, const glm::vec3& normal) {
        const unsigned center = static_cast<unsigned>(m.vertices_.size());
        m.vertices_.push_back({path[ring], normal, glm::vec2(0.5f)});
        const unsigned first = static_cast<unsigned>(m.vertices_.size());
        for (int s = 0; s <= sides; ++s) {
            Vertex v = m.vertices_[ring * stride + s];
            v.normal = normal;
            m.vertices_.push_back(v);
        }
        for (int s = 0; s < sides; ++s)
            m.indices_.insert(m.indices_.end(), {center, first + s, first + s + 1});
    };
    cap(0, -T[0]);
    cap(n - 1, T[n - 1]);

    m.upload();
    return m;
}
