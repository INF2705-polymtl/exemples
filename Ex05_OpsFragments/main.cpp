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
#include <inf2705/TransformStack.hpp>
#include <inf2705/ShaderProgram.hpp>
#include <inf2705/OrbitCamera.hpp>
#include <inf2705/Texture.hpp>


using namespace gl;
using namespace glm;


struct App : public OpenGLApplication
{
	Mesh cube;
	Mesh quad;
	Texture texBox;
	Texture texDrywall;
	Texture texWindow;
	ShaderProgram basicProg;
	ShaderProgram fogProg;

	TransformStack model = {"model"};
	TransformStack view = {"view"};
	TransformStack projection = {"projection"};

	OrbitCamera camera = {5, 30, -30, 0};

	bool showingGlassQuad = false;
	bool showingGlassCube = false;
	bool showingFog = false;
	float fogNear = 2;
	float fogFar = 7;

	// Appelée avant la première trame.
	void init() override {
		setKeybindMessage(
			"R : réinitialiser la position de la caméra." "\n"
			"+ et - :  rapprocher et éloigner la caméra orbitale." "\n"
			"haut/bas : changer la latitude de la caméra orbitale." "\n"
			"gauche/droite : changer la longitude ou le roulement (avec shift) de la caméra orbitale." "\n"
			"clic central (cliquer la roulette) : bouger la caméra en glissant la souris." "\n"
			"roulette : rapprocher et éloigner la caméra orbitale." "\n"
			"1 : activer/désactiver le carré de vitre." "\n"
			"2 : activer/désactiver le cube de vitre." "\n"
			"3 : activer/désactiver le brouillard." "\n"
			"Z, X : rapprocher/éloigner la distance du début du fondu du brouillard." "\n"
			"C, V : rapprocher/éloigner la distance du début du brouillard total." "\n"
			"B : réinitialiser les limites du brouillard." "\n"
		);

		// Pas de cull
		glDisable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

		// Initialiser l'intervalle de distance et la couleur du brouillard.
		fogProg.use();
		fogProg.setVec("fogColor", vec3{0.1f, 0.2f, 0.2f});
		fogProg.setFloat("fogNear", fogNear);
		fogProg.setFloat("fogFar", fogFar);

		cube = Mesh::loadFromWavefrontFile("cube_box.obj")[0];
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
		basicProg.setInt("texMain", 0);
		fogProg.setInt("texMain", 0);

		camera.updateProgram(basicProg, view);
		camera.updateProgram(fogProg, view);
		applyPerspective();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		// On ajoute le bit de profondeur.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Choisir le programme nuanceur à utiliser selon le mode (avec ou sans brouillard). Remarquez que c'est la seule opération à faire pour activer ou non le brouillard. Pas besoin de changer la façon de dessiner la scène.
		auto& currentProg = (showingFog) ? fogProg : basicProg;
		currentProg.use();

		// La façon la plus fiable d'afficher des objets semi transparents dans une scène est d'afficher les objets opaques en premier en activant l'écriture dans le tampon de profondeur. On affiche ensuite les objets translucides en appliquant le test de profondeur mais sans la modification du tampon pour ne pas que les objets transparents se cachent.

		// Activer l'écriture dans le tampon de profondeur.
		glDepthMask(GL_TRUE);
		// Lier la texture de plâtre
		texDrywall.bindToTextureUnit(0);
		// Positionner le paneau applati.
		model.push(); {
			model.translate({0, -1, 0});
			model.scale({1, 0.2f, 2});
			currentProg.setMat(model);
		} model.pop();
		// Dessiner le cube.
		cube.draw();

		// Lier la texture de carton imprimés.
		texBox.bindToTextureUnit(0);
		// Positionner la boîte de carton.
		model.push(); {
			model.translate({0, 0, -1});
			model.scale({0.5f, 0.5f, 0.5f});
			currentProg.setMat(model);
		} model.pop();
		// Dessiner le cube.
		cube.draw();

		// Désactiver l'écriture dans le tampon de profondeur. Le test de profondeur va quand même s'effectuer, mais le tampon ne sera pas modifié.
		glDepthMask(GL_FALSE);
		// Lier la texture de vitre givrée.
		texWindow.bindToTextureUnit(0);

		// Dessiner le cube de vitre si applicable.
		if (showingGlassCube) {
			model.push(); {
				model.translate({0, 0, 1});
				model.scale({0.5f, 0.5f, 0.5f});
				currentProg.setMat(model);
			} model.pop();
			cube.draw();
		}

		// Dessiner le carré de vitre si applicable.
		if (showingGlassQuad) {
			model.push(); {
				model.rotate(-90, {0, 1, 0});
				model.rotate(-15, {1, 0, 0});
				model.translate({0, 0, 3});
				model.scale({1.5f, 1.5f, 1.5f});
				currentProg.setMat(model);
			} model.pop();
			quad.draw();
		}

		// Réactiver l'écriture dans le tampon. Oui, c'est un peu redondant vu qu'on le fait au début de la fonction, mais c'est bon pour la paix d'esprit.
		glDepthMask(GL_TRUE);

		// Malgré nos précautions, on observe quand même une aberration si on regarde le cube de vitre devant le carré de vitre. Ça donne l'impression que le carré est devant le cube. Il n'y a pas de façon magique de régler ce problème juste en manipulant le tampon de profondeur. Il faut soit changer l'ordre d'affichage (coûteux pour les scènes avec beaucoup d'objets) ou appliquer un algorithme plus avancé comme la transparence avec poids ou un tampon de profondeur auxiliaire. De nos jours, la tendance est certainement vers le ray-tracing (pas matière à INF2705) pour gérer les problèmes de transparence/réflexion/réfraction.
	}

	// Appelée lors d'une touche de clavier.
	void onKeyPress(const sf::Event::KeyEvent& key) override {
		// La touche R réinitialise la position de la caméra.
		// Les touches + et - rapprochent et éloignent la caméra orbitale.
		// Les touches haut/bas change l'élévation ou la latitude de la caméra orbitale.
		// Les touches gauche/droite change la longitude ou le roulement (avec shift) de la caméra orbitale.
		camera.handleKeyEvent(key, 5.0f, 0.5f, {5, 30, -30, 0});
		camera.updateProgram(basicProg, view);
		camera.updateProgram(fogProg, view);

		// Touche 1 : Carré de vitre.
		// Touche 2 : Cube de vitre.
		// Touche 3 : Brouillard à la Silent Hill.
		// Touches z, x, c, v, b : Intervalle de distance du brouillard.
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
			std::cout << "Fog near Z : " << fogNear << "\n";
			break;
		case X:
			fogNear += 0.5f;
			std::cout << "Fog near Z : " << fogNear << "\n";
			break;
		case C:
			fogFar -= 0.5f;
			std::cout << "Fog far Z : " << fogFar << "\n";
			break;
		case V:
			fogFar += 0.5f;
			std::cout << "Fog far Z : " << fogFar << "\n";
			break;
		case B:
			fogNear = 2;
			fogFar = 7;
			std::cout << "Fog near Z : " << fogNear << ", "
			          << "Fog far Z : " << fogFar << "\n";
			break;
		}

		fogProg.use();
		fogProg.setFloat("fogNear", fogNear);
		fogProg.setFloat("fogFar", fogFar);
	}

	// Appelée lors d'un mouvement de souris.
	void onMouseMove(const sf::Event::MouseMoveEvent& mouseDelta) override {
		// Mettre à jour la caméra si on a un clic de la roulette.
		auto& mouse = getMouse();
		camera.handleMouseMoveEvent(mouseDelta, mouse, deltaTime_ / (0.7f / 30));
		camera.updateProgram(basicProg, view);
		camera.updateProgram(fogProg, view);
	}

	// Appelée lors d'un défilement de souris.
	void onMouseScroll(const sf::Event::MouseWheelScrollEvent& mouseScroll) override {
		// Zoom in/out
		camera.altitude -= mouseScroll.delta;
		camera.updateProgram(basicProg, view);
		camera.updateProgram(fogProg, view);
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::SizeEvent& event) override {
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
		projection.pushIdentity();
		// Appliquer la perspective avec un champs de vision (FOV) vertical donné et avec un aspect correspondant à celui de la fenêtre.
		projection.perspective(50, getWindowAspect(), 0.01f, 100.0f);
		basicProg.use();
		basicProg.setMat("projection", projection);
		fogProg.use();
		fogProg.setMat("projection", projection);
		projection.pop();
	}
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	// On peut choisir le niveau d'anticrenélage (CSAA) dans la création du contexte OpenGL. SDL a un équivalent pour configurer l'antialias.
	settings.context.antialiasingLevel = 16;
	// Il faut aussi à la création du contexte demander une certaine largeur (taille des valeurs pour chaque fragments) de tampon de profondeur.
	settings.context.depthBits = 24;
	// Allez dans OpenGLApplication::createWindowAndContext pour voir comment ceci est passé à SFML.

	App app;
	app.run(argc, argv, "Exemple Semaine 5: Opérations sur les fragments", settings);
}
