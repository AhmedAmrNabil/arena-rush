#include "collision-debug-drawer.hpp"

#ifdef COLLISION_DEBUG_DRAW

#include <cmath>
#include <cstdio>
#include <glm/gtc/type_ptr.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace gameplay {

    // ---- Inline GLSL shaders (no external files needed) ----

    static const char* kLineVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

out vec3 vColor;

uniform mat4 uVP;

void main() {
    gl_Position = uVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

    static const char* kLineFragmentShader = R"(
#version 330 core
in vec3 vColor;
out vec4 fragColor;

void main() {
    fragColor = vec4(vColor, 1.0);
}
)";

    // ---- Helper: compile a single shader stage ----
    static GLuint compileShaderStage(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[512];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            fprintf(stderr, "[CollisionDebugDrawer] Shader compile error:\n%s\n", log);
        }
        return shader;
    }

    void CollisionDebugDrawer::createShader() {
        GLuint vert = compileShaderStage(GL_VERTEX_SHADER, kLineVertexShader);
        GLuint frag = compileShaderStage(GL_FRAGMENT_SHADER, kLineFragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vert);
        glAttachShader(shaderProgram, frag);
        glLinkProgram(shaderProgram);

        GLint success = 0;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            char log[512];
            glGetProgramInfoLog(shaderProgram, sizeof(log), nullptr, log);
            fprintf(stderr, "[CollisionDebugDrawer] Shader link error:\n%s\n", log);
        }

        // Shaders are linked into the program; we no longer need the individual stage objects
        glDeleteShader(vert);
        glDeleteShader(frag);

        vpUniformLoc = glGetUniformLocation(shaderProgram, "uVP");
    }

    void CollisionDebugDrawer::initialize() {
        createShader();

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // Position attribute (location 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex),
                              reinterpret_cast<void*>(offsetof(LineVertex, x)));

        // Color attribute (location 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex),
                              reinterpret_cast<void*>(offsetof(LineVertex, r)));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void CollisionDebugDrawer::destroy() {
        if (vao) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }
        if (vbo) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
        if (shaderProgram) {
            glDeleteProgram(shaderProgram);
            shaderProgram = 0;
        }
        lineVertices.clear();
    }

    // ---- Wireframe generation helpers ----

    void CollisionDebugDrawer::addLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) {
        lineVertices.push_back({from.x, from.y, from.z, color.r, color.g, color.b});
        lineVertices.push_back({to.x, to.y, to.z, color.r, color.g, color.b});
    }

    void CollisionDebugDrawer::addCircle(const glm::vec3& center, const glm::vec3& axisA, const glm::vec3& axisB,
                                         float radius, const glm::vec3& color, int segments) {
        for (int i = 0; i < segments; i++) {
            float a0 = (float)i / (float)segments * 2.0f * (float)M_PI;
            float a1 = (float)(i + 1) / (float)segments * 2.0f * (float)M_PI;

            glm::vec3 p0 = center + axisA * (radius * cosf(a0)) + axisB * (radius * sinf(a0));
            glm::vec3 p1 = center + axisA * (radius * cosf(a1)) + axisB * (radius * sinf(a1));

            addLine(p0, p1, color);
        }
    }

    void CollisionDebugDrawer::drawSphereWireframe(const glm::mat4& transform, float radius, const glm::vec3& color) {
        glm::vec3 center = glm::vec3(transform[3]);

        // Extract rotated axes (normalized, ignoring scale since Bullet shapes handle scale internally)
        glm::vec3 right = glm::normalize(glm::vec3(transform[0]));
        glm::vec3 up = glm::normalize(glm::vec3(transform[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(transform[2]));

        // Draw 3 orthogonal circles
        addCircle(center, right, forward, radius, color, 32);  // XZ circle (horizontal)
        addCircle(center, right, up, radius, color, 32);       // XY circle
        addCircle(center, forward, up, radius, color, 32);     // ZY circle
    }

    void CollisionDebugDrawer::drawCapsuleWireframe(const glm::mat4& transform, float radius, float totalHeight,
                                                    const glm::vec3& color) {
        glm::vec3 center = glm::vec3(transform[3]);

        glm::vec3 right = glm::normalize(glm::vec3(transform[0]));
        glm::vec3 up = glm::normalize(glm::vec3(transform[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(transform[2]));

        // Capsule spine = totalHeight - 2*radius
        float spine = totalHeight - 2.0f * radius;
        if (spine < 0.0f) spine = 0.0f;
        float halfSpine = spine * 0.5f;

        glm::vec3 topCenter = center + up * halfSpine;
        glm::vec3 bottomCenter = center - up * halfSpine;

        // Horizontal circles at top and bottom of the spine
        addCircle(topCenter, right, forward, radius, color, 32);
        addCircle(bottomCenter, right, forward, radius, color, 32);

        // Vertical side lines connecting top and bottom
        for (int i = 0; i < 8; i++) {
            float angle = (float)i / 8.0f * 2.0f * (float)M_PI;
            glm::vec3 offset = right * (radius * cosf(angle)) + forward * (radius * sinf(angle));
            addLine(topCenter + offset, bottomCenter + offset, color);
        }

        // Semi-circles for the top cap (hemisphere going up)
        int halfSegs = 16;
        for (int i = 0; i < halfSegs; i++) {
            float a0 = (float)i / (float)halfSegs * (float)M_PI;  // 0 to PI (top half)
            float a1 = (float)(i + 1) / (float)halfSegs * (float)M_PI;

            // XY plane semi-circle
            glm::vec3 p0 = topCenter + right * (radius * sinf(a0)) + up * (radius * cosf(a0));
            glm::vec3 p1 = topCenter + right * (radius * sinf(a1)) + up * (radius * cosf(a1));
            addLine(p0, p1, color);

            // ZY plane semi-circle
            p0 = topCenter + forward * (radius * sinf(a0)) + up * (radius * cosf(a0));
            p1 = topCenter + forward * (radius * sinf(a1)) + up * (radius * cosf(a1));
            addLine(p0, p1, color);
        }

        // Semi-circles for the bottom cap (hemisphere going down)
        for (int i = 0; i < halfSegs; i++) {
            float a0 = (float)i / (float)halfSegs * (float)M_PI;
            float a1 = (float)(i + 1) / (float)halfSegs * (float)M_PI;

            // XY plane semi-circle (flipped downward)
            glm::vec3 p0 = bottomCenter + right * (radius * sinf(a0)) - up * (radius * cosf(a0));
            glm::vec3 p1 = bottomCenter + right * (radius * sinf(a1)) - up * (radius * cosf(a1));
            addLine(p0, p1, color);

            // ZY plane semi-circle (flipped downward)
            p0 = bottomCenter + forward * (radius * sinf(a0)) - up * (radius * cosf(a0));
            p1 = bottomCenter + forward * (radius * sinf(a1)) - up * (radius * cosf(a1));
            addLine(p0, p1, color);
        }
    }

    // ---- btIDebugDraw overrides (still needed since Bullet requires them) ----
    void CollisionDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color) {
        lineVertices.push_back({from.getX(), from.getY(), from.getZ(), color.getX(), color.getY(), color.getZ()});
        lineVertices.push_back({to.getX(), to.getY(), to.getZ(), color.getX(), color.getY(), color.getZ()});
    }

    void CollisionDebugDrawer::render(const glm::mat4& VP) {
        if (lineVertices.empty()) return;

        // Upload line data
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(lineVertices.size() * sizeof(LineVertex)),
                     lineVertices.data(), GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Disable depth testing — debug wireframes should always be visible on top.
        // This also avoids issues with postprocess framebuffers leaving stale depth data.
        GLboolean prevDepthTest;
        glGetBooleanv(GL_DEPTH_TEST, &prevDepthTest);
        glDisable(GL_DEPTH_TEST);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(vpUniformLoc, 1, GL_FALSE, glm::value_ptr(VP));

        glBindVertexArray(vao);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineVertices.size()));
        glBindVertexArray(0);

        glUseProgram(0);

        // Restore previous state
        if (prevDepthTest) glEnable(GL_DEPTH_TEST);

        // Clear for next frame
        lineVertices.clear();
    }

}  // namespace gameplay

#endif  // COLLISION_DEBUG_DRAW
