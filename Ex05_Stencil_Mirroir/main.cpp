#include <cstddef>
#include <cstdint>

#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <numbers>

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
	Mesh floor;
	Mesh teapot;
	Mesh cube;
	Mesh pole;
	Mesh quad;
	Mesh mirrorFrame;
	Texture texSteel;
	Texture texRust;
	Texture texConcrete;
	Texture texBox;
	Texture texBuilding;
	Texture texRock;
	Texture texGlass;
	Texture texBlank;
	Texture texStencil;

	ShaderProgram clipPlaneProg;

	TransformStack model = {"model"};
	TransformStack view = {"view"};
	TransformStack projection = {"projection"};

	OrbitCamera camera = {5, 30, 30, 0, {0, 2, 0}};
	float teapotValue = 0;
	float teapotAngle = 0;
	vec3 mirrorPosition = {0, 3, -5};
	bool showingStencil = false;
	bool usingGlassTextured = true;
	bool showingRegularScene = true;
	Uniform<vec4> clipPlane = {"clipPlane"};

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
			"1 : activer/désactiver l'illustration de la zone affectée par le stencil." "\n"
			"2 : activer/désactiver la scène normale (pas réfléchie dans le mirroir)." "\n"
			"3 : activer/désactiver la texture de vitre du mirroir." "\n"
			"W et S : bouger le mirroir en Z." "\n"
			"A et D : bouger le mirroir en X." "\n"
		);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);

		loadShaders();

		teapot = Mesh::loadFromWavefrontFile("teapot.obj")[0];
		cube = Mesh::loadFromWavefrontFile("cube.obj")[0];
		floor = Mesh::loadFromWavefrontFile("floor.obj")[0];
		pole = Mesh::loadFromWavefrontFile("pole.obj")[0];
		quad = Mesh::loadFromWavefrontFile("quad.obj")[0];
		mirrorFrame = Mesh::loadFromWavefrontFile("frame.obj")[0];

		texSteel = Texture::loadFromFile("steel.png", 8);
		texRust = Texture::loadFromFile("rust.png", 8);
		texConcrete = Texture::loadFromFile("concrete.png", 8);
		texBuilding = Texture::loadFromFile("building.png", 8);
		texBox = Texture::loadFromFile("box.png", 1);
		texRock = Texture::loadFromFile("rock.png", 8);
		texGlass = Texture::loadFromFile("glass.png", 8);
		texStencil = Texture::loadFromFile("stencil.png", 1);
		texBlank = Texture::createFromColor({0, 0, 0, 0});
		clipPlaneProg.use();
		clipPlaneProg.setInt("texMain", 0);

		camera.updateProgram(clipPlaneProg, view);
		applyPerspective();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// Calculer l'angle de la théière selon le temps écoulé depuis la dernière trame.
		teapotValue += getFrameDeltaTime();
		teapotValue = fmodf(teapotValue, 5.0f);
		teapotAngle = 20 * sin(teapotValue / 5 * 2*std::numbers::pi_v<float>);

		// Remplir le stencil avec le miroir (on veut des 1 pour tous les pixels du miroir, 0 partout ailleurs), mais sans rien dessiner concrètement. Pour y arriver, on configure le test de stencil pour qu'il ne passe jamais tout en remplissant le stencil de 1.
		glEnable(GL_STENCIL_TEST);
		// Le test ne passe jamais, donc les tampons de couleurs et de profondeur ne sont pas modifiés.
		glStencilFunc(GL_NEVER, 1, 1);
		// Pour chaque pixel du mirroir, remplacer par la valeur de référence (1 dans notre cas).
		glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
		// On a déjà activé le culling pour le programme. Ça fait en sorte que la face arrière du miroir remplisse le stencil. Ça fait en sorte que la face arrière ne fait pas de réflexion. Ça simplifie le reste du code, car on n'a pas besoin de penser au plan de coupe pour les deux faces.
		drawMirrorSurface();

		// Dessiner la scène réfléchie, mais seulement dans la région du mirroir.
		// Activer le test de stencil pour garder seulement ce qui est dans la zone du stencil égale à 1.
		glStencilFunc(GL_EQUAL, 1, 1);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		// Définir les faces avant en ordre horaire (donc l'inverse du mode usuel). En effet, la scène réfléchie est bel et bien un mirroir, donc l'ordre relatif des sommets dessinés (horaire/anti-horaire) est aussi inversé.
		glFrontFace(GL_CW);
		drawReflectedScene();
		if (showingStencil)
			// À fin d'illustration, montrer la zone affectée par le stencil.
			drawStencilZone();
		// Rétablir l'ordre usuel des faces et désactiver le test de stencil.
		glFrontFace(GL_CCW);
		glDisable(GL_STENCIL_TEST);

		// Dessiner la surface du mirroir comme une vitre. D'une part, ça identifie visuellement la surface en lui donnant une texture. D'autre part (plus important), ça met les bonnes valeurs en Z pour la surface. En effet, le tampon de profondeur est jusque là remplit avec les valeurs laissées par la scène réfléchie, alors que le mirroir est supposé être un objet solide dans la scène. On dessine donc une surface plane (même si on lui donne une couleur entièrement transparente) pour remplir le z-buffer correctement avant de dessiner le reste de la scène normalement.
		drawMirrorSurface();

		// Dessiner la scène normalement.
		if (showingRegularScene)
			drawScene();
	}

	// Appelée lors d'une touche de clavier.
	void onKeyPress(const sf::Event::KeyEvent& key) override {
		// La touche R réinitialise la position de la caméra.
		// Les touches + et - rapprochent et éloignent la caméra orbitale.
		// Les touches haut/bas change l'élévation ou la latitude de la caméra orbitale.
		// Les touches gauche/droite change la longitude ou le roulement (avec shift) de la caméra orbitale.

		camera.handleKeyEvent(key, 5, 0.5, {5, 30, 30, 0, {0, 2, 0}});
		camera.updateProgram(clipPlaneProg, view);

		using enum sf::Keyboard::Key;
		switch (key.code) {
		case Num1:
			showingStencil ^= 1;
			std::cout << "Montrer le stencil " << (showingStencil ? "ON" : "OFF") << "\n";
			break;
		case Num2:
			showingRegularScene ^= 1;
			std::cout << "Dessiner la scène normale " << (showingRegularScene ? "ON" : "OFF") << "\n";
			break;
		case Num3:
			usingGlassTextured ^= 1;
			std::cout << "Texture mirroir " << (usingGlassTextured ? "ON" : "OFF") << "\n";
			break;
		case W:
			mirrorPosition.z -= 0.5;
			break;
		case S:
			mirrorPosition.z += 0.5;
			break;
		case A:
			mirrorPosition.x -= 0.5;
			break;
		case D:
			mirrorPosition.x += 0.5;
			break;

		case F5:
			saveScreenshot();
			break;
		}
	}

	// Appelée lors d'un mouvement de souris.
	void onMouseMove(const sf::Event::MouseMoveEvent& mouseDelta) override {
		// Mettre à jour la caméra si on a un clic droit ou central.
		auto& mouse = getMouse();
		camera.handleMouseMoveEvent(mouseDelta, mouse, deltaTime_ / (0.7f / 30));
		camera.updateProgram(clipPlaneProg, view);
	}

	// Appelée lors d'un défilement de souris.
	void onMouseScroll(const sf::Event::MouseWheelScrollEvent& mouseScroll) override {
		// Zoom in/out
		camera.altitude -= mouseScroll.delta;
		camera.updateProgram(clipPlaneProg, view);
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::SizeEvent& event) override {
		applyPerspective();
	}

	void drawMirrorSurface() {
		clipPlaneProg.use();

		// La surface du mirroir.
		model.push(); {
			model.translate(mirrorPosition);
			model.scale({4, 2, 1});
			clipPlaneProg.setMat(model);
		} model.pop();
		if (usingGlassTextured)
			// Texture semi-transparente.
			texGlass.bindToTextureUnit(0);
		else
			// Texture complètement transparente.
			texBlank.bindToTextureUnit(0);
		quad.draw();
	}

	void drawReflectedScene() {
		clipPlaneProg.use();

		// Activer le plan de coupe et définir sa formule à partir de l'équation implicite du plan.
		glEnable(GL_CLIP_PLANE0);
		clipPlane = {0, 0, -1, mirrorPosition.z};
		// La distance au plan de coupe est calculée dans le nuanceur de sommets.
		clipPlaneProg.setUniform(clipPlane);

		model.push(); {
			// Déplacer la scène au complet à la position du mirroir.
			model.translate(mirrorPosition);
			// Appliquer une réflexion (mise à l'échelle de -1) en Z, car le mirroir est dans le plan XY.
			model.scale({1, 1, -1});
			// Appliquer la translation inverse.
			model.translate(-mirrorPosition);
			// Dessiner la scène avec la matrice de modélisation réfléchie.
			drawScene();
		} model.pop();

		// Désactiver le plan de coupe, on n'en veut pas pour le dessin autre que la scène réfléchie.
		glDisable(GL_CLIP_PLANE0);
	}

	void drawScene() {
		clipPlaneProg.use();

		clipPlaneProg.setMat(model);

		// Le plancher en ciment.
		model.push(); {
			model.translate({0, 0, -0.5});
			clipPlaneProg.setMat(model);
		} model.pop();
		texConcrete.bindToTextureUnit(0);
		floor.draw();

		// Le cube qui ressemble à un bâtiment.
		model.push(); {
			model.translate({-4, 1.45, 5});
			model.scale({1, 1.5, 1});
			clipPlaneProg.setMat(model);
		} model.pop();
		texBuilding.bindToTextureUnit(0);
		cube.draw();

		// La grosse boîte de carton.
		model.push(); {
			model.translate({3, 1.45, 4});
			model.rotate(45, {0, 1, 0});
			model.scale({1.5, 1.5, 1.5});
			clipPlaneProg.setMat(model);
		} model.pop();
		texBox.bindToTextureUnit(0);
		cube.draw();

		// Le pole de rotation de la théière.
		model.push(); {
			model.translate({-2.8, 2.8, 4.5});
			model.rotate(90, {1, 0, 0});
			model.scale({0.5, 0.2, 0.5});
			clipPlaneProg.setMat(model);
		} model.pop();
		texRust.bindToTextureUnit(0);
		pole.draw();

		// La théière qui bouge.
		model.push(); {
			model.translate({-2.7, 2.9, 5});
			model.rotate(90, {0, 1, 0});
			model.rotate(teapotAngle, {1, 0, 0});
			model.translate({0, -0.3, 1.5});
			clipPlaneProg.setMat(model);
		} model.pop();
		texRock.bindToTextureUnit(0);
		teapot.draw();

		// Le poteau auquel est attaché le mirroir.
		model.push(); {
			model.translate({mirrorPosition.x, 0, mirrorPosition.z - 0.5});
			model.scale({0.75, 1, 0.75});
			clipPlaneProg.setMat(model);
		} model.pop();
		texRust.bindToTextureUnit(0);
		pole.draw();

		// Le cadre du mirroir.
		model.push(); {
			model.translate(mirrorPosition);
			model.scale({4, 2, 1});
			clipPlaneProg.setMat(model);
		} model.pop();
		texSteel.bindToTextureUnit(0);
		mirrorFrame.draw();
	}

	void drawStencilZone() {
		// Dessiner un quad qui prend toute la fenêtre avec une texture.
		model.pushIdentity(); view.pushIdentity(); projection.pushIdentity(); {
			clipPlaneProg.setMat(model);
			clipPlaneProg.setMat(view);
			clipPlaneProg.setMat(projection);
			texStencil.bindToTextureUnit(0);
			glDisable(GL_CULL_FACE);
			quad.draw();
			glEnable(GL_CULL_FACE);
		} model.pop(); view.pop(); projection.pop();
		clipPlaneProg.setMat(model);
		clipPlaneProg.setMat(view);
		clipPlaneProg.setMat(projection);
	}

	void applyPerspective(float fovy = 50) {
		projection.perspective(fovy, getWindowAspect(), 0.1f, 100.0f);
		clipPlaneProg.setMat(projection);
	}

	void loadShaders() {
		clipPlaneProg.create();
		clipPlaneProg.attachSourceFile(GL_VERTEX_SHADER, "clip_dist_vert.glsl");
		clipPlaneProg.attachSourceFile(GL_FRAGMENT_SHADER, "basic_frag.glsl");
		clipPlaneProg.link();
	}
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	settings.context.antialiasingLevel = 4;

	App app;
	app.run(argc, argv, "Exemple Semaine 5: Mirroir", settings);
}
