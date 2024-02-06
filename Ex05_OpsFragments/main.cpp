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
#include <inf2705/OrbitCamera.hpp>
#include <inf2705/Texture.hpp>


using namespace gl;
using namespace glm;


struct App : public OpenGLApplication
{
	// Appelée avant la première trame.
	void init() override {
		// Config de base, pas de cull, lignes assez visibles.
		glDisable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_POINT_SMOOTH);
		glPointSize(3.0f);
		glLineWidth(3.0f);
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);

		// Activer le test de profondeur.
		glEnable(GL_DEPTH_TEST);
		// Choisir la fonction de comparaison (GL_LESS par défaut)
		glDepthFunc(GL_LESS);
		// Activer la fusion de couleur.
		glEnable(GL_BLEND);
		// Choisir les facteurs de mélange.
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		loadShaders();
		fogProg.use();
		fogProg.setVec("fogColor", vec3{0.1f, 0.2f, 0.2f});
		fogProg.setFloat("fogNear", fogNear);
		fogProg.setFloat("fogFar", fogFar);

		cube = Mesh::loadFromWavefrontFile("cube_box.obj")[0];
		cube.setup();
		quad.vertices = {
			{{-1, -1, 0}, {}, {0, 0}},
			{{ 1, -1, 0}, {}, {1, 0}},
			{{ 1,  1, 0}, {}, {1, 1}},
			{{-1, -1, 0}, {}, {0, 0}},
			{{ 1,  1, 0}, {}, {1, 1}},
			{{-1,  1, 0}, {}, {0, 1}},
		};
		quad.setup();

		texBox = Texture::loadFromFile("box.png", 5);
		texDrywall = Texture::loadFromFile("drywall.png", 5);
		texWindow = Texture::loadFromFile("window.png", 5);

		camera.updateProgram(basicProg, "view", view);
		camera.updateProgram(fogProg, "view", view);
		applyPerspective();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		// On ajoute le bit de stencil.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		auto& currentProg = (showingFog) ? fogProg : basicProg;
		currentProg.use();

		//glColorMask(0, 1, 1, 1);
		glDepthMask(GL_TRUE);
		texDrywall.bindToTextureUnit(0, currentProg, "texDiffuse");
		model.push(); {
			model.translate({0, -1, 0});
			model.scale({1, 0.2f, 2});
			currentProg.setMat("model", model);
		} model.pop();
		cube.draw();
		//glColorMask(1, 1, 1, 1);

		texBox.bindToTextureUnit(0, currentProg, "texDiffuse");
		model.push(); {
			model.translate({0, 0, -1});
			model.scale({0.5f, 0.5f, 0.5f});
			currentProg.setMat("model", model);
		} model.pop();
		cube.draw();

		glDepthMask(GL_FALSE);
		//glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
		texWindow.bindToTextureUnit(0, currentProg, "texDiffuse");

		if (showingGlassCube) {
			model.push(); {
				model.translate({0, 0, 1});
				model.scale({0.5f, 0.5f, 0.5f});
				currentProg.setMat("model", model);
			} model.pop();
			cube.draw();
		}

		if (showingGlassQuad) {
			model.push(); {
				model.rotate(-90, {0, 1, 0});
				model.rotate(-45, {1, 0, 0});
				model.translate({0, 0, 3});
				model.scale({1.5f, 1.5f, 1.5f});
				currentProg.setMat("model", model);
			} model.pop();
			quad.draw();
		}

		glDepthMask(GL_TRUE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	// Appelée lors d'une touche de clavier.
	void onKeyEvent(const sf::Event::KeyEvent& key) {
		// La touche R réinitialise la position de la caméra.
		// Les touches + et - rapprochent et éloignent la caméra orbitale.
		// Les touches haut/bas change l'élévation ou la latitude de la caméra orbitale.
		// Les touches gauche/droite change la longitude ou le roulement (avec shift) de la caméra orbitale.
		camera.handleKeyEvent(key, 5.0f, 0.5f);
		camera.updateProgram(basicProg, "view", view);
		camera.updateProgram(fogProg, "view", view);

		using enum sf::Keyboard::Key;
		switch (key.code) {
		case Num1:
			showingGlassQuad = not showingGlassQuad;
			break;
		case Num2:
			showingGlassCube = not showingGlassCube;
			break;
		case Num3:
			showingFog = not showingFog;
			break;

		case Z:
			fogNear -= 0.5f;
			break;
		case X:
			fogNear += 0.5f;
			break;
		case C:
			fogFar -= 0.5f;
			break;
		case V:
			fogFar += 0.5f;
			break;
		}

		fogProg.use();
		fogProg.setFloat("fogNear", fogNear);
		fogProg.setFloat("fogFar", fogFar);
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::SizeEvent& event) {
		applyPerspective();
	}

	void loadShaders() {
		basicProg.create();
		basicProg.attachSourceFile(GL_VERTEX_SHADER, "basic_vert.glsl");
		basicProg.attachSourceFile(GL_FRAGMENT_SHADER, "basic_frag.glsl");
		basicProg.link();

		fogProg.create();
		fogProg.attachSourceFile(GL_VERTEX_SHADER, "fog_vert.glsl");
		fogProg.attachSourceFile(GL_FRAGMENT_SHADER, "fog_frag.glsl");
		fogProg.link();
	}

	void applyPerspective() {
		// Calculer l'aspect de notre caméra à partir des dimensions de la fenêtre.
		auto windowSize = window_.getSize();
		float aspect = (float)windowSize.x / windowSize.y;

		projection.pushIdentity();
		// Appliquer la perspective avec un champs de vision (FOV) vertical donné et avec un aspect correspondant à celui de la fenêtre.
		projection.perspective(50, aspect, 0.01f, 100.0f);
		basicProg.use();
		basicProg.setMat("projection", projection);
		fogProg.use();
		fogProg.setMat("projection", projection);
		projection.pop();
	}

	Mesh cube;
	Mesh quad;
	Texture texBox;
	Texture texDrywall;
	Texture texWindow;
	ShaderProgram basicProg;
	ShaderProgram fogProg;

	TransformStack model;
	TransformStack view;
	TransformStack projection;

	OrbitCamera camera = {5, 30, -30, 0};

	bool showingGlassQuad = false;
	bool showingGlassCube = false;
	bool showingFog = false;
	float fogNear = 2;
	float fogFar = 10;
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	settings.context.antialiasingLevel = 0;
	settings.context.depthBits = 24;
	settings.context.stencilBits = 8;

	App app;
	app.run(argc, argv, "Exemple Semaine 5: Opérations sur les fragments", settings);
}
