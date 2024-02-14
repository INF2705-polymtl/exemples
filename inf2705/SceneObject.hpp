#pragma once


#include <cstddef>
#include <cstdint>

#include <string>
#include <vector>

#include "Mesh.hpp"
#include "ShaderProgram.hpp"
#include "Texture.hpp"
#include "TransformStack.hpp"


using namespace gl;
using namespace glm;


// Un objet dessinable dans une scène.
struct SceneObject
{
	unsigned id = 0; // ID unique de l'objet
	std::string name; // Nom usuel de l'objet
	Mesh* mesh = nullptr; // Le mesh utilisé
	std::vector<BoundTexture> textures; // Les textures utilisées
	TransformStack modelMat; // La matrice de modélisation propre à l'objet

	void draw(ShaderProgram& prog) {
		static Uniform<mat4> uniform = {"model", mat4(1)};

		prog.use();
		for (auto&& tex : textures)
			tex.bindToProgram(prog);
		prog.setMat(uniform.getLoc(prog), modelMat);
		mesh->draw();
	}
};

