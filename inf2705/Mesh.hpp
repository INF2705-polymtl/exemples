#pragma once


#include <cstddef>
#include <cstdint>

#include <fstream>
#include <string>
#include <vector>

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


using namespace gl;
using namespace glm;


#define CONFIGURE_VERTEX_ATTRIB(index, elemType, member)	\
	glVertexAttribPointer(									\
		index,												\
		decltype(elemType::member)::length(),				\
		GL_FLOAT,											\
		GL_FALSE,											\
		sizeof(elemType),									\
		(void*)offsetof(elemType, member)					\
	);														\
	glEnableVertexAttribArray(index);						\


struct VertexData
{
	vec3 position;
	vec3 normal;
	vec2 texCoords;
};

struct Texture
{
	GLuint id;
	std::string type;
};

struct Mesh
{
	std::vector<VertexData> vertices;
	std::vector<GLuint> indices;
	std::vector<Texture> textures;
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;

	void setup() {
		if (vao == 0)
			glGenVertexArrays(1, &vao);
		if (vbo == 0)
			glGenBuffers(1, &vbo);
		if (ebo == 0)
			glGenBuffers(1, &ebo);

		setupAttribs();
		updateBuffers();
	}

	void draw(GLenum drawMode = GL_TRIANGLES) {
		glBindVertexArray(vao);

		if (not indices.empty())
			glDrawElements(drawMode, indices.size(), GL_UNSIGNED_INT, nullptr);
		else
			glDrawArrays(drawMode, 0, vertices.size());

		glBindVertexArray(0);
	}

	void updateBuffers() {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

		if (not vertices.empty()) {
			int verticesNumBytes = vertices.size() * sizeof(VertexData);
			glBufferData(GL_ARRAY_BUFFER, verticesNumBytes, vertices.data(), GL_STATIC_DRAW);
		}

		if (not indices.empty()) {
			int indicesNumBytes = indices.size() * sizeof(GLuint);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesNumBytes, indices.data(), GL_STATIC_DRAW);
		}

		glBindVertexArray(0);
	}

	void setupAttribs() {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		CONFIGURE_VERTEX_ATTRIB(0, VertexData, position);
		CONFIGURE_VERTEX_ATTRIB(1, VertexData, normal);
		CONFIGURE_VERTEX_ATTRIB(2, VertexData, texCoords);

		glBindVertexArray(0);
	}

	void importDataFromWavefrontFile(std::string_view filename) {
		std::ifstream file(filename.data());

		// TODO: Lire un fichier Wavefront
	}
};

