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
#include <inf2705/Texture.hpp>
#include <inf2705/OrbitCamera.hpp>


using namespace gl;
using namespace glm;


struct App : public OpenGLApplication
{
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

	TransformStack model = {"model"};
	TransformStack view = {"view"};
	TransformStack projection = {"projection"};

	OrbitCamera camera = {10, 30, -30, 0, {0, 1, 0}};

	bool showingScope = false;
	float scopeZoom = 1;
	bool showingScopeWireframe = false;
	bool showingScopeNegative = false;

	// Appelée avant la première trame.
	void init() override {
		setKeybindMessage(
			"F5 : capture d'écran." "\n"
			"R : réinitialiser la position de la caméra." "\n"
			"+ et - :  rapprocher et éloigner la caméra orbitale." "\n"
			"haut/bas : changer la latitude de la caméra orbitale." "\n"
			"gauche/droite : changer la longitude ou le roulement (avec shift) de la caméra orbitale." "\n"
			"clic central (cliquer la roulette) : bouger la caméra en glissant la souris." "\n"
			"roulette : rapprocher et éloigner la caméra orbitale." "\n"
			"1 : lunette" "\n"
			"2 : wireframe à travers la lunette" "\n"
			"3 : filtre négatif à travers la lunette" "\n"
			"X et Z : zoom in/out dans la lunette" "\n"
		);

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
		road = Mesh::loadFromWavefrontFile("cube_road.obj")[0];
		teapot = Mesh::loadFromWavefrontFile("teapot.obj")[0];
		// Un quad qui, sans opérations de transformations, prend l'écran au complet (-1 à 1 en xy, donc tout l'écran en coords normalisées). Ça devient important plus tard.
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
		// Activer la répétition pour la texture d'asphalte.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		basicProg.use();
		basicProg.setInt("texMain", 0);

		// La texture de cercle blanc va servir de masque (ou de pochoir) pour la scène avec grossissement.
		texScopeMask = Texture::loadFromFile("scope_mask.png");
		texScopeReticle = Texture::loadFromFile("scope_crosshair.png");

		// Bouger la caméra vers le haut pour que le centre de l'orbite soit un peu au-dessus du plan xz. On le fait avant d'appliquer la caméra orbitale pour bouger le système d'axe.
		applyCamera();
		applyPerspective();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		// On efface maintenant aussi le tampon de pochoir (stencil).
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		basicProg.use();

		// Résumé des opérations pour afficher un réticule dans une lunette qui grossit la scène :
		//    - Dessiner le masque de la lunette (cercle blanc) en mettant seulement à jour le tampon de stencil, mais pas le tampon de couleur. On désactive aussi le test et l'écriture dans le tampon de profondeur et on n'utilise pas les matrices de visualisation ou de projection.
		//    - Dessiner la scène alternative (avec grossissement, wireframe, négatif) en activant le test de stencil mais sans modifier le tampon de stencil. On teste pour la valeur 1 (celle écrite par le cercle blanc précédent).
		//    - Dessiner la scène régulière en appliquant le test de stencil, mais en testant pour des 0.
		//    - Dessiner le réticule (cercle avec croix) sans test de stencil, sans écriture ou test de profondeur, et sans matrices de visualisation ou de projection.

		if (showingScope) {
			// Désactiver l'écriture dans le tampon de couleur et activer l'écriture dans le tampon de stencil. De cette façon, on ne dessine rien à l'écran mais on crée un masque dans le stencil.
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			// Écrire des 1 dans le stencil pour les fragments dessinés. Au départ, il n'y a que des 0 (fait par glClear).
			glStencilFunc(GL_ALWAYS, 1, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			// Désactiver le test de profondeur et l'écriture du tampon de profondeur.
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			// Dessiner le masque.
			drawScopeMask();
			// Rétablir les états.
			glDepthMask(GL_TRUE);
			glEnable(GL_DEPTH_TEST);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

			// Activer le test de stencil et ne pas écrire dans le tampon de stencil.
			glEnable(GL_STENCIL_TEST);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			// Tester pour des 1, ce qui a été mis dans le stencil par le cercle précédent.
			glStencilFunc(GL_EQUAL, 1, 0xFF);
			if (showingScopeWireframe)
				// Si on veut du wireframe, changer pour GL_LINE.
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			// Dessiner la scène en appliquant une perspective plus mince (facteur scopeZoom).
			projection.push();
			applyPerspective(50 / scopeZoom);
			drawScene();
			// Rétablir la matrice de projection.
			projection.pop();
			basicProg.setMat(projection);
			if (showingScopeWireframe)
				// Rétablir à GL_FILL
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			if (showingScopeNegative) {
				// Pour afficher en négatif, il faut activer les opérations logiques et choisir le XOR. A XOR 1 = ~A. Il faut ensuite dessiner encore le cercle blanc (donc des composantes avec tous les bits à 1) et le XOR va s'appliquer sur la scène dans la lunette. Ça donne le négatif de la couleur.
				glLogicOp(GL_XOR);
				glEnable(GL_COLOR_LOGIC_OP);
				// Pour afficher par-dessus tout, il faut désactiver le test de profondeur.
				glDisable(GL_DEPTH_TEST);
				// Dessiner le cercle blanc de la lunette. Le test de stencil est toujours actif.
				drawScopeMask();
				glEnable(GL_DEPTH_TEST);
				glDisable(GL_COLOR_LOGIC_OP);
			}
		}

		// Dessiner la scène normalement en testant pour des 0 dans le stencil.
		glStencilFunc(GL_EQUAL, 0, 0xFF);
		drawScene();

		if (showingScope) {
			// Afficher un overlay de réticule en désactivant le stencil et la profondeur.
			glDisable(GL_STENCIL_TEST);
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			drawCrosshairs();
			// Rétablir l'état.
			glDepthMask(GL_TRUE);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_STENCIL_TEST);
		}
	}

	// Appelée lors d'une touche de clavier.
	void onKeyPress(const sf::Event::KeyEvent& key) override {
		// La touche R réinitialise la position de la caméra.
		// Les touches + et - rapprochent et éloignent la caméra orbitale.
		// Les touches haut/bas change l'élévation ou la latitude de la caméra orbitale.
		// Les touches gauche/droite change la longitude ou le roulement (avec shift) de la caméra orbitale.
		camera.handleKeyEvent(key, 5, 0.5f, {10, 30, -30, 0, {0, 1, 0}});
		applyCamera();

		// Touche 1 : La lunette
		// Touche 2 : Le wireframe à travers la lunette
		// Touche 3 : Filtre négatif à travers la lunette
		// Touche Z : Zoom out
		// Touche X : Zoom in
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

		case F5:
			std::string path = saveScreenshot();
			std::cout << "Capture d'écran dans " << path << std::endl;
			break;
		}
	}

	// Appelée lors d'un mouvement de souris.
	void onMouseMove(const sf::Event::MouseMoveEvent& mouseDelta) override {
		// Mettre à jour la caméra si on a un clic de la roulette.
		auto& mouse = getMouse();
		camera.handleMouseMoveEvent(mouseDelta, mouse, deltaTime_ / (0.7f / 30));
		applyCamera();
	}

	// Appelée lors d'un défilement de souris.
	void onMouseScroll(const sf::Event::MouseWheelScrollEvent& mouseScroll) override {
		// Zoom in/out
		camera.altitude -= mouseScroll.delta;
		applyCamera();
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::SizeEvent& event) override {
		applyPerspective();
	}

	void drawScene() {
		// Il ne se passe rien d'intéressant ici. On positionne et dessine des objets avec leurs textures comme on fait depuis un bout de temps. On a isolé les opérations de dessin de la scène dans cette fonction de façon à pouvoir l'appeler en modifiant tout autour (les matrices de visu/proj, le stencil, le z-buffer).

		basicProg.use();

		texRoad.bindToTextureUnit(0);
		model.push(); {
			model.translate({0, -1, 0});
			model.scale({4, 1, 4});
			model.translate({0, 0, -1});
			basicProg.setMat(model);
		} model.pop();
		road.draw();

		texBuilding.bindToTextureUnit(0);
		model.push(); {
			model.translate({2, 0, 0});
			basicProg.setMat(model);
		} model.pop();
		cube.draw();

		texBuilding.bindToTextureUnit(0);
		model.push(); {
			model.translate({-2, 1, 3});
			model.rotate(90, {0, 1, 0});
			model.scale({1.1f, 2.0f, 1.1f});
			basicProg.setMat(model);
		} model.pop();
		cube.draw();

		texBox.bindToTextureUnit(0);
		model.push(); {
			model.translate({0, 2, -10});
			model.scale({2.5f, 3, 2.5f});
			basicProg.setMat(model);
		} model.pop();
		cube.draw();

		texDrywall.bindToTextureUnit(0);
		model.push(); {
			model.translate({0, 1, -10});
			model.scale({3.2f, 3.2f, 3.2f});
			model.translate({0, 2.0f, 0});
			basicProg.setMat(model);
		} model.pop();
		teapot.draw();
	}

	void drawScopeMask() {
		// À la base, le quad est -1 à 1 en xy, donc prend la fenêtre au complet.
		model.pushIdentity();
		// Faire un scale de 50% et prendre moins de place dans la fenêtre. Pour conserver une forme ronde, on applique l'aspect de la fenêtre au facteur en x.
		model.scale({0.5f / getWindowAspect(), 0.5f, 1});
		basicProg.setMat(model);
		// Passer des matrices identités vu qu'on ne fait rien avec les autres matrices de transformation.
		basicProg.setMat("view", mat4(1));
		basicProg.setMat("projection", mat4(1));
		// Activer l'élimination de pixels invisibles. Voir commentaire dans le nuanceur de fragments.
		basicProg.setBool("shouldDiscard", true);
		// Dessiner le quad avec sa texture.
		texScopeMask.bindToTextureUnit(0);
		quad.draw();
		// Rétablir l'état et les matrices.
		basicProg.setBool("shouldDiscard", false);
		model.pop();
		basicProg.setMat(view);
		basicProg.setMat(projection);
	}

	void drawCrosshairs() {
		// Même concept que dans drawScopeMask, mais on ne fait pas le test de pixel transparent.
		model.pushIdentity();
		model.scale({0.5f / getWindowAspect(), 0.5f, 1});
		basicProg.setMat(model);
		basicProg.setMat("view", mat4(1));
		basicProg.setMat("projection", mat4(1));
		texScopeReticle.bindToTextureUnit(0);
		quad.draw();
		model.pop();
		basicProg.setMat(view);
		basicProg.setMat(projection);
	}

	void applyCamera() {
		view.loadIdentity();
		camera.applyToView(view);
		basicProg.use();
		basicProg.setMat(view);
	}

	void applyPerspective(float vfov = 50) {
		// Appliquer la perspective avec un champs de vision (FOV) vertical donné et avec un aspect correspondant à celui de la fenêtre.
		projection.loadIdentity();
		projection.perspective(vfov, getWindowAspect(), 0.01f, 100.0f);
		basicProg.use();
		basicProg.setMat(projection);
	}

	void loadShaders() {
		basicProg.create();
		basicProg.attachSourceFile(GL_VERTEX_SHADER, "basic_vert.glsl");
		basicProg.attachSourceFile(GL_FRAGMENT_SHADER, "discard_frag.glsl");
		basicProg.link();
	}
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	settings.context.antialiasingLevel = 8;
	// Spécifier la largeur des valeurs du tampon de stencil. Sur beaucoup de matériel, on a généralement 8 bit de stencil. Dans l'exemple d'aujourd'hui, on a seulement besoin d'un bit (0 ou 1).
	settings.context.stencilBits = 1;

	App app;
	app.run(argc, argv, "Exemple Semaine 5: Stencil", settings);
}
