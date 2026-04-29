#pragma once

#include <glad/gl.h>

#include <vector>

#include "vertex.hpp"

namespace our {

#define ATTRIB_LOC_POSITION 0
#define ATTRIB_LOC_COLOR 1
#define ATTRIB_LOC_TEXCOORD 2
#define ATTRIB_LOC_NORMAL 3
#define ATTRIB_LOC_TANGENT 4
#define ATTRIB_LOC_BONE_IDS 5
#define ATTRIB_LOC_BONE_WEIGHTS 6

    class Mesh {
        // Here, we store the object names of the 3 main components of a mesh:
        // A vertex array object, A vertex buffer and an element buffer
        unsigned int VBO = 0;
        unsigned int EBO = 0;
        unsigned int VAO = 0;
        bool vaoReady = false;
        // We need to remember the number of elements that will be draw by glDrawElements
        GLsizei elementCount;
        GLsizei vertexCount;
        std::vector<Vertex> vertices;  // Store vertices for potential future use (e.g., collision, CPU-side processing)
        std::vector<unsigned int> indices;  // Store indices for potential future use

    public:
        GLsizei getVertexCount() const {
            return vertexCount;
        }

        GLsizei getIndexCount() const {
            return elementCount;
        }
        // The constructor takes two vectors:
        // - vertices which contain the vertex data.
        // - elements which contain the indices of the vertices out of which each rectangle will be constructed.
        Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& elements) {
            // Generate VBO and send data
            glGenBuffers(1, &VBO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

            // Generate EBO and send indices of vertices
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(unsigned int), elements.data(),
                         GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            elementCount = static_cast<GLsizei>(elements.size());
            vertexCount = static_cast<GLsizei>(vertices.size());
            this->vertices = vertices;
            this->indices = elements;
        }

        void setupVAO() {
            if (vaoReady) return;

            glGenVertexArrays(1, &VAO);
            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

            glVertexAttribPointer(ATTRIB_LOC_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  (void*)offsetof(Vertex, position));
            glEnableVertexAttribArray(ATTRIB_LOC_POSITION);

            glVertexAttribPointer(ATTRIB_LOC_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex),
                                  (void*)offsetof(Vertex, color));
            glEnableVertexAttribArray(ATTRIB_LOC_COLOR);

            glVertexAttribPointer(ATTRIB_LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  (void*)offsetof(Vertex, tex_coord));
            glEnableVertexAttribArray(ATTRIB_LOC_TEXCOORD);

            glVertexAttribPointer(ATTRIB_LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  (void*)offsetof(Vertex, normal));
            glEnableVertexAttribArray(ATTRIB_LOC_NORMAL);

            glVertexAttribPointer(ATTRIB_LOC_TANGENT, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  (void*)offsetof(Vertex, tangent));
            glEnableVertexAttribArray(ATTRIB_LOC_TANGENT);

            glVertexAttribIPointer(ATTRIB_LOC_BONE_IDS, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, bone_ids));
            glEnableVertexAttribArray(ATTRIB_LOC_BONE_IDS);

            glVertexAttribPointer(ATTRIB_LOC_BONE_WEIGHTS, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  (void*)offsetof(Vertex, weights));
            glEnableVertexAttribArray(ATTRIB_LOC_BONE_WEIGHTS);

            glBindVertexArray(0);
            vaoReady = true;
        }

        // this function should render the mesh
        void draw() {
            if (!vaoReady) setupVAO();
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_INT, 0);
        }

        void update(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& elements) {
            if (!vaoReady) setupVAO();
            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(unsigned int), elements.data(),
                         GL_DYNAMIC_DRAW);

            elementCount = static_cast<GLsizei>(elements.size());

            glBindVertexArray(0);
        }

        // this function should delete the vertex & element buffers and the vertex array object
        ~Mesh() {
            if (VBO) glDeleteBuffers(1, &VBO);
            if (EBO) glDeleteBuffers(1, &EBO);
            if (VAO) glDeleteVertexArrays(1, &VAO);
        }

        const std::vector<Vertex>& getVertices() const {
            return vertices;
        }

        const std::vector<unsigned int>& getIndices() const {
            return indices;
        }

        Mesh(Mesh const&) = delete;
        Mesh& operator=(Mesh const&) = delete;
    };

}  // namespace our
