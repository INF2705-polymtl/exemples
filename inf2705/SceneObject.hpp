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


struct SceneObject
{
	unsigned id = 0;
	std::string name;
	Mesh* mesh = nullptr;
	std::vector<BoundTexture> textures;
	TransformStack modelMat;

	void draw(ShaderProgram& prog) {
		static Uniform<mat4> uniform = {"model", mat4(1)};

		prog.use();
		for (auto&& tex : textures)
			tex.bindToProgram(prog);
		prog.setMat(uniform.getLoc(prog), modelMat);
		mesh->draw();
	}
};

