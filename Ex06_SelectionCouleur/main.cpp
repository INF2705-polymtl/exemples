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
#include <inf2705/OrbitCamera.hpp>
#include <inf2705/ShaderProgram.hpp>
#include <inf2705/Texture.hpp>
#include <inf2705/TransformStack.hpp>
#include <inf2705/SceneObject.hpp>


using namespace gl;
using namespace glm;


inline vec4 uintToVec4(uint32_t i) {
	uint8_t b1 = (i >>  0) & 0xFF;
	uint8_t b2 = (i >>  8) & 0xFF;
	uint8_t b3 = (i >> 16) & 0xFF;
	return vec4(b1 / 255.0f, b2 / 255.0f, b3 / 255.0f, 1);
}

inline uint32_t vec4ToUInt(vec4 v) {
	uint8_t b1 = (uint8_t)std::lrint(v.r * 255);
	uint8_t b2 = (uint8_t)std::lrint(v.g * 255);
	uint8_t b3 = (uint8_t)std::lrint(v.b * 255);
	return (b1 << 0) & (b2 << 8) & (b3 << 16);
}

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

		flashingProg.use();
		flashingProg.setVec("flashingColor", vec4{1, 0.2f, 0.2f, 1});

		meshBoard = Mesh::loadFromWavefrontFile("cube_board.obj")[0];
		meshTeapot = Mesh::loadFromWavefrontFile("teapot.obj")[0];
		texCheckers = Texture::loadFromFile("checkers_board.png", 4);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		texRock = Texture::loadFromFile("rock.png", 4);
		texRockDark = Texture::loadFromFile("rock_dark.png", 4);

		loadScene();

		for (auto&& prog : programs)
			camera.updateProgram(*prog, "view", view);
		applyPerspective();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (selecting) {
			vec4 clearColor = {};
			glGetFloatv(GL_COLOR_CLEAR_VALUE, glm::value_ptr(clearColor));
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_BLEND);
			glDisable(GL_MULTISAMPLE);
			drawScene();
			glEnable(GL_MULTISAMPLE);
			glEnable(GL_BLEND);
			glFinish();

			auto clickedObj = getObjectIDUnderMouse(lastMouseEvent);
			if (clickedObj != 0)
				std::cout << "Clicked object " << clickedObj << "\n";
			if (pieces.contains(clickedObj) and clickedObj != selectedObjectID) {
				selectedObjectID = clickedObj;
				flashingValue = 0;
			}

			selecting = false;
			glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		flashingValue += deltaTime_;
		flashingValue = fmodf(flashingValue, 1.0f);
		flashingProg.use();
		flashingProg.setFloat("flashingValue", 1 - pow(flashingValue, 2.0f));

		drawScene();
	}

	// Appelée lors d'une touche de clavier.
	void onKeyPress(const sf::Event::KeyEvent& key) override {
		camera.handleKeyEvent(key, 5, 0.5f, {10, 90, 180, 0});

		mat4 pieceTranslate(1);
		using enum sf::Keyboard::Key;
		switch (key.code) {
		case Space:
			selectedObjectID = 0;
			break;
		case A:
			pieceTranslate = glm::translate(mat4(1), {1, 0, 0});
			break;
		case D:
			pieceTranslate = glm::translate(mat4(1), {-1, 0, 0});
			break;
		case W:
			pieceTranslate = glm::translate(mat4(1), {0, 0, 1});
			break;
		case S:
			pieceTranslate = glm::translate(mat4(1), {0, 0, -1});
			break;
		}

		auto it = objects.find(selectedObjectID);
		auto* selectedObject = it != objects.end() ? &it->second : nullptr;
		if (selectedObject != nullptr)
			selectedObject->modelMat = pieceTranslate * selectedObject->modelMat.top();

		for (auto&& prog : programs)
			camera.updateProgram(*prog, "view", view);
	}

	// Appelée lors d'un bouton de souris appuyé.
	void onMouseButtonPress(const sf::Event::MouseButtonEvent& mouseBtn) override {
		switch (mouseBtn.button) {
		case sf::Mouse::Left:
			lastMouseEvent = mouseBtn;
			selecting = true;
			break;
		case sf::Mouse::Right:
			selectedObjectID = 0;
			selecting = false;
			break;
		}
	}

	// Appelée lors d'un mouvement de souris.
	void onMouseMove(const sf::Event::MouseMoveEvent& mouseDelta) override {
		auto mouse = getMouse();
		camera.handleMouseMoveEvent(mouseDelta, mouse, deltaTime_ / (0.7f/30));
		for (auto&& prog : programs)
			camera.updateProgram(*prog, "view", view);
	}

	// Appelée lors d'un défilement de souris.
	void onMouseScroll(const sf::Event::MouseWheelScrollEvent& mouseScroll) override {
		camera.altitude -= mouseScroll.delta;
		for (auto&& prog : programs)
			camera.updateProgram(*prog, "view", view);
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::SizeEvent& event) override {
		applyPerspective();
	}

	void drawScene() {
		for (auto&& [objectID, obj] : objects) {
			ShaderProgram* prog = &basicProg;
			if (selecting)
				prog = &selectionProg;
			else if (not selecting and obj.id == selectedObjectID)
				prog = &flashingProg;

			prog->use();
			prog->setUInt("objectID", obj.id);
			prog->setVec("objectColor", uintToVec4(obj.id));

			obj.modelMat.push();
			obj.modelMat.top() = model * obj.modelMat;
			obj.draw(*prog);
			obj.modelMat.pop();
		}
	}

	void loadScene() {
		for (int i = 0; i < 12; i++) {
			for (int j = 0; j < 2; j++) {
				int id = 1000 * (j + 1) + i;
				SceneObject obj = {
					(unsigned)id,
					"Piece",
					&meshTeapot,
					{{(j == 0) ? &texRock : &texRockDark, 0, "texDiffuse"}},
					{}
				};
				if (j == 1)
					obj.modelMat.rotate(180, {0, 1, 0});
				obj.modelMat.translate({(i % 4) * 2 - (i / 4) % 2, 0, i / 4});
				obj.modelMat.translate({-2.5f, -0.15f, -3.5f});
				obj.modelMat.scale({0.3f, 0.3f, 0.3f});
				objects[id] = std::move(obj);
				pieces[id] = &objects.at(id);
			}
		}

		objects[1] = {
			1,
			"Board",
			&meshBoard,
			{{&texCheckers, 0, "texDiffuse"}},
			{}
		};
		board = &objects.at(1);
		board->modelMat.translate({0, -0.5f, 0});
		board->modelMat.scale({4, 0.12f, 4});
	}

	unsigned getObjectIDUnderMouse(const sf::Event::MouseButtonEvent& mouseBtn) {
		struct { GLint x, y, width, height; } viewport = {};
		glGetIntegerv(GL_VIEWPORT, (GLint*)&viewport);
		GLint x = mouseBtn.x;
		GLint y = viewport.height - mouseBtn.y;
		unsigned objectID = 0;
		glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &objectID);
		return objectID;
	}

	void loadShaders() {
		basicProg.create();
		basicProg.attachSourceFile(GL_VERTEX_SHADER, "basic_vert.glsl");
		basicProg.attachSourceFile(GL_FRAGMENT_SHADER, "basic_frag.glsl");
		basicProg.link();
		selectionProg.create();
		selectionProg.attachSourceFile(GL_VERTEX_SHADER, "basic_vert.glsl");
		selectionProg.attachSourceFile(GL_FRAGMENT_SHADER, "selection_frag.glsl");
		selectionProg.link();
		flashingProg.create();
		flashingProg.attachSourceFile(GL_VERTEX_SHADER, "basic_vert.glsl");
		flashingProg.attachSourceFile(GL_FRAGMENT_SHADER, "flashing_frag.glsl");
		flashingProg.link();
	}

	void applyPerspective(float fovy = 50) {
		projection.pushIdentity();
		// Appliquer la perspective avec un champs de vision (FOV) vertical donné et avec un aspect correspondant à celui de la fenêtre.
		projection.perspective(fovy, getWindowAspect(), 0.01f, 100.0f);
		for (auto&& prog : programs) {
			prog->use();
			prog->setMat("projection", projection);
		}
		projection.pop();
	}

	Mesh meshBoard;
	Mesh meshTeapot;
	Texture texRock;
	Texture texRockDark;
	Texture texCheckers;

	std::unordered_map<unsigned, SceneObject> objects;
	std::unordered_map<unsigned, SceneObject*> pieces;
	SceneObject* board = nullptr;

	ShaderProgram basicProg;
	ShaderProgram selectionProg;
	ShaderProgram flashingProg;
	ShaderProgram* programs[3] = {&basicProg, &selectionProg, &flashingProg};

	TransformStack model;
	TransformStack view;
	TransformStack projection;

	OrbitCamera camera = {10, 90, 180, 0};

	sf::Event::MouseButtonEvent lastMouseEvent = {};
	bool selecting = false;
	unsigned selectedObjectID = 0;
	float flashingValue = 0;
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	settings.context.antialiasingLevel = 8;

	App app;
	app.run(argc, argv, "Exemple Semaine 6: Sélection 3D par couleur", settings);
}
