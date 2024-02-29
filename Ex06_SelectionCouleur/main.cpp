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
	auto bytes = (uint8_t*)&i;
	return vec4(bytes[0] / 255.0f, bytes[1] / 255.0f, bytes[2] / 255.0f, 1);
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

		// Affecter une couleur qui va faire clignoter l'objet sélectionné.
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

		// Appliquer la caméra synthétique et la projection perspective.
		for (auto&& prog : programs)
			camera.updateProgram(*prog, "view", view);
		applyPerspective();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Si on est mode de sélection (la trame suivant un clic de souris).
		if (selecting) {
			// Sauvegarder la couleur de fond actuelle.
			vec4 clearColor = {};
			glGetFloatv(GL_COLOR_CLEAR_VALUE, glm::value_ptr(clearColor));
			// Mettre un fond noir.
			glClearColor(0, 0, 0, 1);
			// Vider les tampons de couleurs (avec du noir) et de profondeur.
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			// Désactiver la fusion de couleur. Pour la sélection, on veut des couleurs pleines, uniformes et distinctes. Donc pas de transparence non plus.
			glDisable(GL_BLEND);
			// Désactiver l'anticrenélage. Ça permet de ne pas avoir des erreurs aux bordures des objets.
			glDisable(GL_MULTISAMPLE);
			// Si on avait de l'éclairage, on ne l'appliquerait pas non plus.

			// Dessiner la scène.
			drawScene();

			// Avant de lire le buffer de couleurs, il faut attendre que le dessinage soit fait. glFinish bloque jusqu'à la complétion des commandes envoyées au GPU. Généralement, on n'a pas besoin de faire cet appel car le buffer swap (Window::display avec SFML) le fait pour nous.
			glFinish();

			// Obtenir la couleur du pixel cliqué avec la souris. Cet couleur est un nombre 24 bit (RGB8, on ne peut pas utiliser le alpha) qui correspond à notre identifiant d'objet de scène.
			unsigned clickedObj = getObjectIDUnderMouse(lastMouseBtnEvent);
			if (clickedObj != 0)
				std::cout << "Clicked object " << clickedObj << "\n";
			// Chercher dans le dictionnaire de théières (pieces) si le ID est reconnu. On choisit de ne pas pouvoir sélectionner la planche.
			if (clickedObj != selectedObjectID and pieces.contains(clickedObj)) {
				// Sélectionner l'objet et réinitialiser l'animation de clignotement.
				selectedObjectID = clickedObj;
				flashingValue = 0;
			}

			// Rétablir la configuration régulière.
			selecting = false;
			glEnable(GL_MULTISAMPLE);
			glEnable(GL_BLEND);
			glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		// Mettre à jour la valeur de clignotement, qui est tout simplement le temps en secondes modulo 1.
		flashingValue += deltaTime_;
		flashingValue = fmodf(flashingValue, 1.0f);
		flashingProg.use();
		flashingProg.setFloat("flashingValue", flashingValue);

		// Dessiner la scène normalement.
		drawScene();
	}

	// Appelée lors d'une touche de clavier.
	void onKeyPress(const sf::Event::KeyEvent& key) override {
		// La touche R réinitialise la position de la caméra.
		// Les touches + et - rapprochent et éloignent la caméra orbitale.
		// Les touches haut/bas change l'élévation ou la latitude de la caméra orbitale.
		// Les touches gauche/droite change la longitude ou le roulement (avec shift) de la caméra orbitale.
		// Les touches WASD contrôle la théière sélectionnée.

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

		// Appliquer la translation à l'objet sélectionner si aplicable.
		auto it = objects.find(selectedObjectID);
		auto* selectedObject = it != objects.end() ? &it->second : nullptr;
		if (selectedObject != nullptr)
			selectedObject->modelMat = pieceTranslate * selectedObject->modelMat.top();

		// Mettre à jour la caméra.
		for (auto&& prog : programs)
			camera.updateProgram(*prog, "view", view);
	}

	// Appelée lors d'un bouton de souris appuyé.
	void onMouseButtonPress(const sf::Event::MouseButtonEvent& mouseBtn) override {
		switch (mouseBtn.button) {
		// Clic gauche sélectionne l'objet sous la souris.
		case sf::Mouse::Left:
			// Enregistrer l'évènement de souris et se mettre en mode de sélection.
			lastMouseBtnEvent = mouseBtn;
			selecting = true;
			break;
		// Clic droit déselectionne.
		case sf::Mouse::Right:
			// ID 0 -> pas d'objets sélectionné.
			selectedObjectID = 0;
			selecting = false;
			break;
		}
	}

	// Appelée lors d'un mouvement de souris.
	void onMouseMove(const sf::Event::MouseMoveEvent& mouseDelta) override {
		// Mettre à jour la caméra si on a un clic de la roulette.
		auto mouse = getMouse();
		camera.handleMouseMoveEvent(mouseDelta, mouse, deltaTime_ / (0.7f/30));
		for (auto&& prog : programs)
			camera.updateProgram(*prog, "view", view);
	}

	// Appelée lors d'un défilement de souris.
	void onMouseScroll(const sf::Event::MouseWheelScrollEvent& mouseScroll) override {
		// Zoom in/out
		camera.altitude -= mouseScroll.delta;
		for (auto&& prog : programs)
			camera.updateProgram(*prog, "view", view);
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::SizeEvent& event) override {
		applyPerspective();
	}

	void drawScene() {
		// Pour chaque objet dans la scène. On fait du beau C++17 avec de l'affectation structurée pour itérer sur un hash map (INF1015 to the MOON!).
		for (auto&& [objectID, obj] : objects) {
			// Choisir le programme nuanceur à utiliser (texturé, sélection, ou animation sélectionnée).
			ShaderProgram* prog = &basicProg;
			if (selecting)
				prog = &selectionProg;
			else if (not selecting and obj.id == selectedObjectID)
				prog = &flashingProg;

			// Passer au nuanceur le ID de l'objet et sa couleur convertie.
			prog->use();
			prog->setUint("objectID", obj.id);
			prog->setVec("objectColor", uintToVec4(obj.id));

			// Appliquer la matrice de modélisation globale à celle de l'objet (en restaurant après le dessin).
			obj.modelMat.push();
			obj.modelMat.top() = model * obj.modelMat;
			// Dessiner l'objet normalement en passant le programme nuanceur à utiliser.
			obj.draw(*prog);
			obj.modelMat.pop();
		}
	}

	void loadScene() {
		// Il y a 24 pièces de dame, 12 foncées et 12 pâles.
		for (int i = 0; i < 12; i++) {
			// j=0 -> pièce blanche, j=1 -> pièce noire.
			for (int j = 0; j < 2; j++) {
				// Calculer le ID de l'objet (partant de 1000 pour les pièces blanches, 2000 pour les noires).
				int id = 1000 * (j + 1) + i;
				SceneObject obj = {
					(unsigned)id,
					"Piece",
					&meshTeapot,
					{{(j == 0) ? &texRock : &texRockDark, 0, "texDiffuse"}},
					{}
				};
				// L'origine est au centre du damier.
				if (j == 1)
					// Les pièces noires se retrouve du côté opposé.
					obj.modelMat.rotate(180, {0, 1, 0});
				// Positionner la théière au milieu de la case au coin noir (sinon elle est à l'intersection des cases).
				obj.modelMat.translate({-2.5f, -0.15f, -3.5f});
				// Positionner la théière dans sa case selon son numéro.
				obj.modelMat.translate({(i % 4) * 2 - (i / 4) % 2, 0, i / 4});
				// Mise à l'échelle uniforme à 30% pour entrer dans les cases.
				obj.modelMat.scale({0.3f, 0.3f, 0.3f});
				// Enregistrer l'objet et en mettre un pointeur dans le dictionnaire de pièces (pour les séparer des objets non-sélectionnables).
				objects[id] = std::move(obj);
				pieces[id] = &objects.at(id);
			}
		}

		// Le damier (la planche de jeu) a le numéro 1 et est centrer à l'origine de la scène.
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
		// Du bon vieux C où on crée une structure anonyme plutôt qu'un tableau bête.
		struct { GLint x, y, width, height; } viewport = {};
		// Obtenir le viewport pour convertir les coordonnées de souris (référentiel haut-gauche) au référentiel bas-gauche.
		glGetIntegerv(GL_VIEWPORT, (GLint*)&viewport);
		GLint x = mouseBtn.x;
		GLint y = viewport.height - mouseBtn.y;

		// Lire la couleur sous la souris. On initialise un entier 32bit à 0 qu'on passe comme pointeur de données à glReadPixels. On demande les composantes RGB (pas le alpha), ce qui va remplir seulement les 24 premiers bits de objectID, nous donnant notre identifiant (couleur = ID dans notre cas).
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

	sf::Event::MouseButtonEvent lastMouseBtnEvent = {};
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
