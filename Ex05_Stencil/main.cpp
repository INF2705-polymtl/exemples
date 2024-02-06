#include <cstddef>
#include <cstdint>

#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#define GLM_FORCE_SWIZZLE // Pour utiliser les .xyz .rb etc. comme avec GLSL.
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <inf2705/OpenGLApplication.hpp>
#include <inf2705/Mesh.hpp>
#include <inf2705/TransformStack.hpp>
#include <inf2705/ShaderProgram.hpp>
#include <inf2705/Texture.hpp>
#include <inf2705/OrbitCamera.hpp>


using namespace gl;
using namespace glm;


struct App : public OpenGLApplication
{
	// Appelée avant la première trame.
	void init() override {
		// Config de base, pas de cull, lignes assez visibles.
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_POINT_SMOOTH);
		glPointSize(3.0f);
		glLineWidth(3.0f);
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);

		loadShaders();

		cube = Mesh::loadFromWavefrontFile("cube_box.obj")[0];
		cube.setup();
		road = Mesh::loadFromWavefrontFile("cube_road.obj")[0];
		road.setup();
		teapot = Mesh::loadFromWavefrontFile("teapot.obj")[0];
		teapot.setup();
		quad.vertices = {
			{{-1, -1, 0}, {}, {0, 0}},
			{{ 1, -1, 0}, {}, {1, 0}},
			{{ 1,  1, 0}, {}, {1, 1}},
			{{-1, -1, 0}, {}, {0, 0}},
			{{ 1,  1, 0}, {}, {1, 1}},
			{{-1,  1, 0}, {}, {0, 1}},
		};
		quad.setup();

		texBox = Texture::loadFromFile("box.png");
		texBuilding = Texture::loadFromFile("building.png");
		texDrywall = Texture::loadFromFile("drywall.png");
		texRoad = Texture::loadFromFile("asphalt.png");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		texScopeMask = Texture::loadFromFile("scope_mask.png");
		texScopeReticle = Texture::loadFromFile("scope_crosshair.png");

		view.translate({0, -2, 0});
		camera.updateProgram(basicProg, "view", view);
		applyPerspective();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		basicProg.use();

		if (showingScope) {
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(GL_ALWAYS, 1, 1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			drawScopeMask();
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glStencilFunc(GL_EQUAL, 1, 1);
			camera.updateProgram(basicProg, "view", view);
			applyPerspective(50 / scopeZoom);
			if (showingScopeWireframe)
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			drawScene();
			if (showingScopeWireframe)
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			if (showingScopeNegative) {
				glLogicOp(GL_XOR);
				glEnable(GL_COLOR_LOGIC_OP);
				drawScopeMask();
				glDisable(GL_COLOR_LOGIC_OP);
			}
			glStencilFunc(GL_EQUAL, 0, 1);

			camera.updateProgram(basicProg, "view", view);
			applyPerspective(50);
		}

		drawScene();

		if (showingScope) {
			glDisable(GL_STENCIL_TEST);
			drawCrosshairs();
			camera.updateProgram(basicProg, "view", view);
			applyPerspective(50);
		}
	}

	// Appelée lors d'une touche de clavier.
	void onKeyEvent(const sf::Event::KeyEvent& key) {
		camera.handleKeyEvent(key, 5, 0.5f);
		camera.updateProgram(basicProg, "view", view);

		using enum sf::Keyboard::Key;
		switch (key.code) {
		case Num1:
			showingScope = not showingScope;
			break;
		case Num2:
			showingScopeWireframe = not showingScopeWireframe;
			break;
		case Num3:
			showingScopeNegative = not showingScopeNegative;
			break;
		case Z:
			scopeZoom -= 1;
			scopeZoom = std::max(scopeZoom, 1.0f);
			break;
		case X:
			scopeZoom += 1;
			break;
		}
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::SizeEvent& event) {
		applyPerspective();
	}

	void drawScene() {
		basicProg.use();

		texRoad.bindToTextureUnit(0, basicProg, "texDiffuse");
		model.push(); {
			model.translate({0, -1, 0});
			model.scale({4, 1, 4});
			model.translate({0, 0, -1});
			basicProg.setMat("model", model);
		} model.pop();
		road.draw();

		texBuilding.bindToTextureUnit(0, basicProg, "texDiffuse");
		model.push(); {
			model.translate({2, 0, 2});
			basicProg.setMat("model", model);
		} model.pop();
		cube.draw();

		texBuilding.bindToTextureUnit(0, basicProg, "texDiffuse");
		model.push(); {
			model.translate({-2, 0, 3});
			basicProg.setMat("model", model);
		} model.pop();
		cube.draw();

		texBox.bindToTextureUnit(0, basicProg, "texDiffuse");
		model.push(); {
			model.translate({0, 2, -10});
			model.scale({2.5f, 3, 2.5f});
			basicProg.setMat("model", model);
		} model.pop();
		cube.draw();

		texDrywall.bindToTextureUnit(0, basicProg, "texDiffuse");
		model.push(); {
			model.translate({0, 1, -10});
			model.scale({3, 3, 3});
			model.translate({0, 2, 0});
			basicProg.setMat("model", model);
		} model.pop();
		teapot.draw();
	}

	void drawScopeMask() {
		basicProg.setMat("model", glm::scale(mat4(1), vec3{0.5f / getWindowAspect(), 0.5f, 1}));
		basicProg.setMat("view", mat4(1));
		basicProg.setMat("projection", mat4(1));
		basicProg.setBool("shouldDiscard", true);
		texScopeMask.bindToTextureUnit(0, basicProg, "texDiffuse");
		glDisable(GL_DEPTH_TEST);
		quad.draw();
		glEnable(GL_DEPTH_TEST);
		basicProg.setBool("shouldDiscard", false);
	}

	void drawCrosshairs() {
		basicProg.setMat("model", glm::scale(mat4(1), vec3{0.5f / getWindowAspect(), 0.5f, 1}));
		basicProg.setMat("view", mat4(1));
		basicProg.setMat("projection", mat4(1));
		texScopeReticle.bindToTextureUnit(0, basicProg, "texDiffuse");
		glDisable(GL_DEPTH_TEST);
		quad.draw();
		glEnable(GL_DEPTH_TEST);
	}

	void applyPerspective(float vfov = 50) {
		projection.pushIdentity();
		// Appliquer la perspective avec un champs de vision (FOV) vertical donné et avec un aspect correspondant à celui de la fenêtre.
		projection.perspective(vfov, getWindowAspect(), 0.01f, 100.0f);
		basicProg.use();
		basicProg.setMat("projection", projection);
		projection.pop();
	}

	void loadShaders() {
		basicProg.create();
		basicProg.attachSourceFile(GL_VERTEX_SHADER, "vert.glsl");
		basicProg.attachSourceFile(GL_FRAGMENT_SHADER, "frag.glsl");
		basicProg.link();
	}

	Mesh teapot;
	Mesh cube;
	Mesh road;
	Mesh quad;
	Texture texBox;
	Texture texBuilding;
	Texture texDrywall;
	Texture texRoad;
	Texture texScopeReticle;
	Texture texScopeMask;

	ShaderProgram basicProg;

	TransformStack model;
	TransformStack view;
	TransformStack projection;

	OrbitCamera camera = {10, 30, -30, 0};

	bool showingScope = false;
	float scopeZoom = 1;
	bool showingScopeWireframe = false;
	bool showingScopeNegative = false;
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	settings.context.antialiasingLevel = 8;
	settings.context.stencilBits = 1;

	App app;
	app.run(argc, argv, "Exemple Semaine 5: Stencil", settings);
}
