#pragma once

#ifdef COLLISION_DEBUG_DRAW

#include <LinearMath/btIDebugDraw.h>
#include <glad/gl.h>

#include <glm/glm.hpp>
#include <vector>

namespace gameplay {

    class CollisionDebugDrawer : public btIDebugDraw {
    public:
        void initialize();
        void destroy();

        // Call after the main renderer finishes. Uploads the accumulated lines and draws them.
        void render(const glm::mat4& VP);

        // ---- Manual wireframe generators (used instead of Bullet's debugDrawObject) ----
        void drawSphereWireframe(const glm::mat4& transform, float radius, const glm::vec3& color);
        void drawBoxWireframe(const glm::mat4& transform, const glm::vec3& halfExtents, const glm::vec3& color);
        void drawCapsuleWireframe(const glm::mat4& transform, float radius, float totalHeight, const glm::vec3& color);

        // ---- btIDebugDraw overrides (required by interface) ----
        void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
        void drawContactPoint(const btVector3&, const btVector3&, btScalar, int, const btVector3&) override {}
        void reportErrorWarning(const char*) override {}
        void draw3dText(const btVector3&, const char*) override {}
        void setDebugMode(int mode) override {
            debugMode = mode;
        }
        int getDebugMode() const override {
            return debugMode;
        }

    private:
        struct LineVertex {
            float x, y, z;  // position
            float r, g, b;  // color
        };

        std::vector<LineVertex> lineVertices;

        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint shaderProgram = 0;
        GLint vpUniformLoc = -1;

        int debugMode = DBG_DrawWireframe;

        void createShader();
        void addLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);
        void addCircle(const glm::vec3& center, const glm::vec3& axisA, const glm::vec3& axisB, float radius,
                       const glm::vec3& color, int segments = 32);
    };

}  // namespace gameplay

#endif  // COLLISION_DEBUG_DRAW
