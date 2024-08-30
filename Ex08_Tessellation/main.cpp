#include <cstddef>
#include <cstdint>

#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <inf2705/OpenGLApplication.hpp>
#include <inf2705/Mesh.hpp>
#include <inf2705/ShaderProgram.hpp>
#include <inf2705/Texture.hpp>
#include <inf2705/TransformStack.hpp>
#include <inf2705/OrbitCamera.hpp>


using namespace gl;
using namespace glm;


struct App : public OpenGLApplication
{
	Mesh d20;
	Texture texBox;

	ShaderProgram uniColorProg;
	ShaderProgram sphereProg;

	TransformStack model = {"model"};
	TransformStack view = {"view"};
	TransformStack projection = {"projection"};

	OrbitCamera camera = {5, 30, 30, 0};

	bool wireframeMode = false;
	bool cullMode = true;
	bool showingOriginalShape = false;
	Uniform<int> tessLevelInner = {"tessLevelInner", 1};
	Uniform<int> tessLevelOuter = {"tessLevelOuter", 1};

	// Appelée avant la première trame.
	void init() override {
		setKeybindMessage(
			"F5 : capture d'écran." "\n"
			"R : réinitialiser la position de la caméra." "\n"
			"+ et - :  rapprocher et éloigner la caméra orbitale." "\n"
			"haut/bas : changer la latitude de la caméra orbitale." "\n"
			"gauche/droite : changer la longitude ou le roulement (avec shift) de la caméra orbitale." "\n"
			"clic droit ou central : bouger la caméra en glissant la souris." "\n"
			"roulette : rapprocher et éloigner la caméra orbitale." "\n"
			"1 : afficher en faces pleines ou wireframe." "\n"
			"2 : montrer ou cacher les arêtes de la forme originale (avant tessellation)." "\n"
			"3 : activer/désactiver le cull des faces arrières." "\n"
			"W et S : étirer/compresser le d20 en Y." "\n"
			"A et D : étirer/compresser le d20 en XZ." "\n"
			"I et shift+I : augmenter/diminuer le niveau de tessellation intérieur." "\n"
			"O et shift+O : augmenter/diminuer le niveau de tessellation extérieur." "\n"
		);

		// Config de base, pas de cull, lignes assez visibles.
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_POINT_SMOOTH);
		glEnable(GL_LINE_SMOOTH);
		glPointSize(5.0f);
		glLineWidth(3.0f);
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);

		loadShaders();

		d20 = Mesh::loadFromWavefrontFile("d20.obj")[0];

		texBox = Texture::loadFromFile("box_bg.png");
		texBox.bindToTextureUnit(0);
		sphereProg.use();
		sphereProg.setInt("texMain", 0);

		uniColorProg.use();
		uniColorProg.setVec("globalColor", vec4(1, 1, 1, 1));

		camera.updateProgram(sphereProg, view);
		camera.updateProgram(uniColorProg, view);
		applyPerspective();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		sphereProg.use();
		sphereProg.setMat(model);
		glPolygonMode(GL_FRONT_AND_BACK, (wireframeMode) ? GL_LINE : GL_FILL);
		if (cullMode)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);
		// Configurer le nombre de sommets par patch. Ici on choisit 3 pour traiter chaque triangle comme un patch (plus simple pour ce qu'on veut faire).
		glPatchParameteri(GL_PATCH_VERTICES, 3);
		// Dessiner le d20 en utilisant GL_PATCHES plutôt que le GL_TRIANGLES auquel on est habitué.
		d20.draw(GL_PATCHES);

		// Dessiner la forme originale pour mieux illustrer la tessellation.
		if (showingOriginalShape) {
			uniColorProg.use();
			uniColorProg.setMat(model);
			glDisable(GL_DEPTH_TEST);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			d20.draw(GL_TRIANGLES);
			glEnable(GL_DEPTH_TEST);
		}
	}

	// Appelée lorsque la fenêtre se ferme.
	void onClose() override {
		d20.deleteObjects();
		texBox.deleteObject();
		for (auto prog : {&uniColorProg, &sphereProg}) {
			prog->deleteShaders();
			prog->deleteProgram();
		}
	}

	// Appelée lors d'une touche de clavier.
	void onKeyPress(const sf::Event::KeyEvent& key) override {
		// La touche R réinitialise la position de la caméra.
		// Les touches + et - rapprochent et éloignent la caméra orbitale.
		// Les touches haut/bas change l'élévation ou la latitude de la caméra orbitale.
		// Les touches gauche/droite change la longitude ou le roulement (avec shift) de la caméra orbitale.
		//
		// 1 : Afficher en faces pleines ou wireframe.
		// 2 : Montrer ou cacher les arêtes de la forme originale (avant tessellation).
		// 3 : Activer/désactiver le cull des faces arrières.
		// W et S : Étirer/compresser le D20 en Y.
		// A et D : Étirer/compresser le D20 en XZ.
		// I et shift+I : augmenter/diminuer le niveau de tessellation intérieur.
		// O et shift+O : augmenter/diminuer le niveau de tessellation extérieur.

		camera.handleKeyEvent(key, 5, 0.5, {5, 30, 30, 0});
		camera.updateProgram(sphereProg, view);
		camera.updateProgram(uniColorProg, view);

		using enum sf::Keyboard::Key;
		switch (key.code) {
		case A:
			model.scale({1.1f, 1, 1.1f});
			//std::cout << "Scale XZ : " << model.top()[0][0] << "\n";
			break;
		case D:
			model.scale({0.9f, 1, 0.9f});
			//std::cout << "Scale XZ : " << model.top()[0][0] << "\n";
			break;
		case W:
			model.scale({1, 1.1f, 1});
			//std::cout << "Scale Y : " << model.top()[1][1] << "\n";
			break;
		case S:
			model.scale({1, 0.9f, 1});
			//std::cout << "Scale Y : " << model.top()[1][1] << "\n";
			break;

		case Num1:
			wireframeMode ^= 1;
			std::cout << "Wireframe " << (wireframeMode ? "ON" : "OFF") << "\n";
			break;
		case Num2:
			showingOriginalShape ^= 1;
			std::cout << "Mesh original visible " << (showingOriginalShape ? "ON" : "OFF") << "\n";
			break;
		case Num3:
			cullMode ^= 1;
			std::cout << "Cull " << (cullMode ? "ON" : "OFF") << "\n";
			break;

		case I:
			if (key.shift)
				tessLevelInner -= 1;
			else
				tessLevelInner += 1;
			tessLevelInner = std::max(tessLevelInner.get(), 1);
			sphereProg.use();
			sphereProg.setUniform(tessLevelInner);
			std::cout << std::format(
				"Niveau tess intérieur={} extérieur={}",
				tessLevelInner.get(), tessLevelOuter.get()
			) << "\n";
			break;
		case O:
			if (key.shift)
				tessLevelOuter -= 1;
			else
				tessLevelOuter += 1;
			tessLevelOuter = std::max(tessLevelOuter.get(), 1);
			sphereProg.use();
			sphereProg.setUniform(tessLevelOuter);
			std::cout << std::format(
				"Niveau tess intérieur={} extérieur={}",
				tessLevelInner.get(), tessLevelOuter.get()
			) << "\n";
			break;

		case F5:
			std::string path = saveScreenshot();
			std::cout << "Capture d'écran dans " << path << std::endl;
			break;
		}
	}

	// Appelée lors d'un mouvement de souris.
	void onMouseMove(const sf::Event::MouseMoveEvent& mouseDelta) override {
		// Mettre à jour la caméra si on a un clic droit ou central.
		auto& mouse = getMouse();
		camera.handleMouseMoveEvent(mouseDelta, mouse, deltaTime_ / (0.7f / 30));
		camera.updateProgram(sphereProg, view);
		camera.updateProgram(uniColorProg, view);
	}

	// Appelée lors d'un défilement de souris.
	void onMouseScroll(const sf::Event::MouseWheelScrollEvent& mouseScroll) override {
		// Zoom in/out
		camera.altitude -= mouseScroll.delta;
		camera.updateProgram(sphereProg, view);
		camera.updateProgram(uniColorProg, view);
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::SizeEvent& event) override {
		applyPerspective();
	}

	void applyPerspective(float fovy = 50) {
		// Appliquer la perspective avec un champs de vision (FOV) vertical donné et avec un aspect correspondant à celui de la fenêtre.
		projection.perspective(fovy, getWindowAspect(), 0.01f, 100.0f);
		sphereProg.use();
		sphereProg.setUniform(projection);
		uniColorProg.use();
		uniColorProg.setUniform(projection);
	}

	void loadShaders() {
		uniColorProg.attachSourceFile(GL_VERTEX_SHADER, "basic_vert.glsl");
		uniColorProg.attachSourceFile(GL_FRAGMENT_SHADER, "uniform_frag.glsl");
		uniColorProg.link();

		sphereProg.attachSourceFile(GL_VERTEX_SHADER, "sphere_vert.glsl");
		sphereProg.attachSourceFile(GL_TESS_CONTROL_SHADER, "sphere_tessctrl.glsl");
		sphereProg.attachSourceFile(GL_TESS_EVALUATION_SHADER, "sphere_tesseval.glsl");
		sphereProg.attachSourceFile(GL_GEOMETRY_SHADER, "sphere_geom.glsl");
		sphereProg.attachSourceFile(GL_FRAGMENT_SHADER, "basic_frag.glsl");
		sphereProg.link();
	}
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	settings.context.antialiasingLevel = 4;

	App app;
	app.run(argc, argv, "Exemple Semaine 8: Nuanceurs de tessellation", settings);
}
