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

#include "utils.hpp"


using namespace gl;
using namespace glm;


// Configurer l'attribut de sommets pour un membre scalaire `member` (int, float) d'une struct `elemType`.
#define SET_SCALAR_VERTEX_ATTRIB_FROM_STRUCT_MEM(index, elemType, member)	\
{																			\
	glVertexAttribPointer(													\
		(GLuint)index,														\
		1,																	\
		getTypeGLenum_v<decltype(elemType::member)>,						\
		GL_FALSE,															\
		(GLint)sizeof(elemType),											\
		(const void*)offsetof(elemType, member)								\
	);																		\
	glEnableVertexAttribArray(index);										\
}																			\

// Configurer l'attribut de sommets pour un membre vectoriel `member` (vec3, vec4) d'une struct `elemType`.
#define SET_VEC_VERTEX_ATTRIB_FROM_STRUCT_MEM(index, elemType, member)	\
{																		\
	glVertexAttribPointer(												\
		(GLuint)index,													\
		(GLint)decltype(elemType::member)::length(),					\
		getTypeGLenum_v<decltype(elemType::member)::value_type>,		\
		GL_FALSE,														\
		(GLint)sizeof(elemType),										\
		(const void*)offsetof(elemType, member)							\
	);																	\
	glEnableVertexAttribArray(index);									\
}																		\


// Informations de base d'un sommet
struct VertexData
{
	vec3 position;  // layout(location = 0)
	vec3 normal;    // layout(location = 1)
	vec2 texCoords; // layout(location = 2)
};

// Un mesh (ou maillage) représente la géométrie d'un objet d'une façon traçable par OpenGL.
struct Mesh
{
	std::vector<VertexData> vertices;
	std::vector<GLuint> indices;
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;

	void setup(GLenum usageMode = GL_STATIC_DRAW) {
		// Créer les buffer objects.
		if (vao == 0)
			glGenVertexArrays(1, &vao);
		if (vbo == 0)
			glGenBuffers(1, &vbo);
		if (ebo == 0)
			glGenBuffers(1, &ebo);

		// Mettre les données dans les tampons en mémoire graphique.
		updateBuffers(usageMode);
		// Configurer les attributs selon la struct VertexData.
		setupAttribs();
	}

	void draw(GLenum drawMode = GL_TRIANGLES) {
		bindVao();

		// Avoir un tableau d'indices vide ou non indique si on veut dessiner avec les données directement ou avec un tableau de connectivité.
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
		glDrawArrays(drawMode, offset, (GLsizei)vertices.size());
	}

	void drawElements(GLenum drawMode, GLsizei numIndices, GLsizei offset = 0) {
		// Techniquement, on n'a pas besoin de refaire les glBindBuffer, mais ça ne coûte pas cher et c'est plus fiable de les refaire.
		bindEbo();
		// Tracer selon le tampon d'indices.
		glDrawElements(drawMode, numIndices, GL_UNSIGNED_INT, (const void*)(size_t)offset);
	}

	void updateBuffers(GLenum usageMode = GL_STATIC_DRAW) {
		bindVao();
		bindVbo();
		bindEbo();

		if (not vertices.empty()) {
			auto numBytes = vertices.size() * sizeof(VertexData);
			glBufferData(GL_ARRAY_BUFFER, numBytes, vertices.data(), usageMode);
		}
		if (not indices.empty()) {
			auto numBytes = indices.size() * sizeof(GLuint);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, numBytes, indices.data(), usageMode);
		}

		unbindVao();
	}

	void setupAttribs() {
		bindVao();
		bindVbo();

		// Les données des sommets (positions, normales, coords de textures) sont placées ensembles dans le même tampon, de façon contigües. Les attributs sont configurés pour accéder à un membre de VertexData dans chaque élément.
		SET_VEC_VERTEX_ATTRIB_FROM_STRUCT_MEM(0, VertexData, position);
		SET_VEC_VERTEX_ATTRIB_FROM_STRUCT_MEM(1, VertexData, normal);
		SET_VEC_VERTEX_ATTRIB_FROM_STRUCT_MEM(2, VertexData, texCoords);

		unbindVao();
	}

	void deleteObjects() {
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
		vao = vbo = ebo = 0;
	}

	void bindVao() { glBindVertexArray(vao); }
	void unbindVao() { glBindVertexArray(0); }
	void bindVbo() { glBindBuffer(GL_ARRAY_BUFFER, vbo); }
	void bindEbo() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); }

	// Charge des mesh d'objets à partir d'un fichier Wavefront (il peut y avoir plusieurs objet dans le même fichier). Les données sont chargées par sommet sans tableau d'indices.
	static std::vector<Mesh> loadFromWavefrontFile(std::string_view filename, bool setupOnLoad = true) {
		// Code inspiré de l'exemple https://github.com/tinyobjloader/tinyobjloader/tree/release#example-code-new-object-oriented-api

		// Lire le fichier et vérifier les erreurs. On le charge en spécifiant à tinyobjloader de faire la séparation en triangles des faces non triangulaires (des quadrilatères par exemple).
		tinyobj::ObjReader reader;
		tinyobj::ObjReaderConfig config = {};
		config.triangulate = true;
		if (not reader.ParseFromFile(filename.data(), config)) {
			std::cerr << "ERROR tinyobj::ObjReader: " << reader.Error();
			return {};
		}
		if (not reader.Warning().empty())
			std::cerr << "WARNING tinyobj::ObjReader: " << reader.Warning();

		std::vector<Mesh> result;

		// Pour chaque objet défini dans le fichier:
		for (auto&& shape : reader.GetShapes()) {
			Mesh mesh;

			size_t index_offset = 0;
			// Pour chaque face:
			for (auto&& numVertices : shape.mesh.num_face_vertices) {
				// Pour chaque sommet de la face:
				for (size_t v = 0; v < numVertices; v++) {
					VertexData data = {};
					// Obtenir les indices des éléments du sommet.
					tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
					auto& attribs = reader.GetAttrib();

					// Copier la position.
					data.position = *(const vec3*)&attribs.vertices[3 * size_t(idx.vertex_index)];
					// Copier la normale si l'index de normales est positif.
					if (idx.normal_index >= 0)
						data.normal = normalize(*(const vec3*)&attribs.normals[3 * size_t(idx.normal_index)]);
					// Copier les coordonnées de texture si l'index est positif.
					if (idx.texcoord_index >= 0)
						data.texCoords = *(const vec2*)&attribs.texcoords[2 * size_t(idx.texcoord_index)];

					// Ajouter le sommet au tableau de sommets.
					mesh.vertices.push_back(data);
				}
				index_offset += numVertices;
			}

			if (setupOnLoad)
				mesh.setup();
			result.push_back(std::move(mesh));
		}

		return result;
	}
};

