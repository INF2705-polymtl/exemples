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


// Utilisation légitime de macros : génération de code
#define SET_VERTEX_ATTRIB_FROM_STRUCT_MEM(index, elemType, member)	\
	glVertexAttribPointer(											\
		index,														\
		(GLint)decltype(elemType::member)::length(),				\
		GL_FLOAT,													\
		GL_FALSE,													\
		(GLint)sizeof(elemType),									\
		(const void*)offsetof(elemType, member)						\
	);																\
	glEnableVertexAttribArray(index);								\


// Informations de base d'un sommet
struct VertexData
{
	vec3 position;
	vec3 normal;
	vec2 texCoords;
};

// Informations de base d'une texture chargée
struct Texture
{
	GLuint id;
	std::string type;
};

// Un mesh représente la géométrie d'une forme d'une façon traçable par OpenGL.
struct Mesh
{
	std::vector<VertexData> vertices;
	std::vector<GLuint> indices;
	std::vector<Texture> textures;
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;

	void setup() {
		// Créer les buffer objects.
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
		bindVao();

		// Avoir un tableau d'incices vide ou non indique si on veut dessiner avec le tableau directement ou avec les indices.
		if (not indices.empty())
			drawElements(drawMode, (GLsizei)indices.size(), nullptr);
		else
			drawArrays(drawMode);

		unbindVao();
	}

	void drawArrays(GLenum drawMode) {
		// Techniquement, on n'a pas besoin de refaire les glBindBuffer, mais ça ne coûte pas cher et c'est plus fiable de les refaire.
		bindVbo();
		// Tracer selon le tampon de données.
		glDrawArrays(drawMode, 0, (GLint)vertices.size());
	}

	void drawElements(GLenum drawMode, GLsizei numIndices, const GLuint* subIndices) {
		// Techniquement, on n'a pas besoin de refaire les glBindBuffer, mais ça ne coûte pas cher et c'est plus fiable de les refaire.
		bindEbo();
		// Tracer selon le tampon d'indices.
		glDrawElements(drawMode, numIndices, GL_UNSIGNED_INT, subIndices);
	}

	void updateBuffers() {
		bindVao();
		bindVbo();
		bindEbo();

		if (not vertices.empty()) {
			auto numBytes = vertices.size() * sizeof(VertexData);
			glBufferData(GL_ARRAY_BUFFER, (GLint)numBytes, vertices.data(), GL_STATIC_DRAW);
		}
		if (not indices.empty()) {
			auto numBytes = indices.size() * sizeof(GLuint);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLint)numBytes, indices.data(), GL_STATIC_DRAW);
		}

		unbindVao();
	}

	void setupAttribs() {
		bindVao();
		bindVbo();

		// Les données des sommets (positions, normales, coords de textures) sont placées ensembles dans le même tampon, de façon contigües. Les attributs sont configurés pour accéder à une position dans chaque élément
		SET_VERTEX_ATTRIB_FROM_STRUCT_MEM(0, VertexData, position);
		SET_VERTEX_ATTRIB_FROM_STRUCT_MEM(1, VertexData, normal);
		SET_VERTEX_ATTRIB_FROM_STRUCT_MEM(2, VertexData, texCoords);

		unbindVao();
	}

	void bindVao() { glBindVertexArray(vao); }
	void unbindVao() { glBindVertexArray(0); }
	void bindVbo() { glBindBuffer(GL_ARRAY_BUFFER, vbo); }
	void bindEbo() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); }

	void importDataFromWavefrontFile(std::string_view filename) {
		std::ifstream file(filename.data());

		// TODO: Lire un fichier Wavefront
	}
};

