#pragma once


#include <cstddef>
#include <cstdint>

#include <fstream>
#include <string>
#include <vector>

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tiny_obj_loader.h>


using namespace gl;
using namespace glm;


// Utilisation légitime de macros : génération de code
#define SET_VERTEX_ATTRIB_FROM_STRUCT_MEM(index, elemType, member)	\
	glVertexAttribPointer(											\
		(GLuint)index,												\
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
	vec3 position;  // layout(location = 0)
	vec3 normal;    // layout(location = 1)
	vec2 texCoords; // layout(location = 2)
};

// Un mesh représente la géométrie d'une forme d'une façon traçable par OpenGL.
struct Mesh
{
	std::vector<VertexData> vertices;
	std::vector<GLuint> indices;
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

		updateBuffers();
		setupAttribs();
	}

	void draw(GLenum drawMode = GL_TRIANGLES) {
		bindVao();

		// Avoir un tableau d'incices vide ou non indique si on veut dessiner avec le tableau directement ou avec les indices.
		if (not indices.empty())
			drawElements(drawMode, (GLsizei)indices.size());
		else
			drawArrays(drawMode);

		unbindVao();
	}

	void drawArrays(GLenum drawMode, GLint offset = 0) {
		// Techniquement, on n'a pas besoin de refaire les glBindBuffer, mais ça ne coûte pas cher et c'est plus fiable de les refaire.
		bindVbo();
		// Tracer selon le tampon de données.
		glDrawArrays(drawMode, offset, (GLint)vertices.size());
	}

	void drawElements(GLenum drawMode, GLsizei numIndices, GLsizei offset = 0) {
		// Techniquement, on n'a pas besoin de refaire les glBindBuffer, mais ça ne coûte pas cher et c'est plus fiable de les refaire.
		bindEbo();
		// Tracer selon le tampon d'indices.
		glDrawElements(drawMode, numIndices, GL_UNSIGNED_INT, (const void*)(size_t)offset);
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

	static std::vector<Mesh> loadFromWavefrontFile(std::string_view filename) {
		// Code inspiré de l'exemple https://github.com/tinyobjloader/tinyobjloader/tree/release#example-code-new-object-oriented-api

		tinyobj::ObjReader reader;

		if (not reader.ParseFromFile(filename.data(), {})) {
			std::cerr << "ERROR tinyobj::ObjReader: " << reader.Error();
			return {};
		}

		if (!reader.Warning().empty())
			std::cerr << "WARNING tinyobj::ObjReader: " << reader.Warning();

		std::vector<Mesh> result;

		for (auto&& shape : reader.GetShapes()) {
			Mesh mesh;

			size_t index_offset = 0;
			for (auto&& fv : shape.mesh.num_face_vertices) {
				for (size_t v = 0; v < fv; v++) {
					VertexData data = {};
					tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
					auto& attribs = reader.GetAttrib();

					// positions
					data.position = *(const vec3*)&attribs.vertices[3 * size_t(idx.vertex_index)];
					// Check if `normal_index` is zero or positive. negative = no normal data
					if (idx.normal_index >= 0)
						data.normal = *(const vec3*)&attribs.normals[3 * size_t(idx.normal_index)];
					// Check if `texcoord_index` is zero or positive. negative = no texcoord data
					if (idx.texcoord_index >= 0)
						data.texCoords = *(const vec2*)&attribs.texcoords[2 * size_t(idx.texcoord_index)];

					mesh.vertices.push_back(data);
				}
				index_offset += fv;
			}
			
			result.push_back(std::move(mesh));
		}

		return result;
	}
};

