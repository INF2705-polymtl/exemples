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
	Mesh sphere;
	Mesh eye;
	Mesh quad;
	Mesh tv;
	Texture texSteel;
	Texture texRust;
	Texture texEye;
	Texture texConcrete;
	Texture texBox;
	Texture texBuilding;
	Texture texRock;

	Texture texRender;
	GLuint camFrameBuffer;
	GLuint camZBuffer;

	ShaderProgram basicProg;

	TransformStack model = {"model"};
	TransformStack view = {"view"};
	TransformStack projection = {"projection"};

	OrbitCamera camera = {10, 15, 30, 0, {0, 2, -5}};
	float scanValue = 0;
	float scanAngle = 0;
	bool scanPaused = false;
	float teapotValue = 0;

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
			"espace : mettre en pause le mouvement de la caméra de surveillance." "\n"
		);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);

		// On utilise les nuanceurs de base qui échantillonnent les textures, pas besoin de quoique ce soit de fancy.
		loadShaders();

		teapot = Mesh::loadFromWavefrontFile("teapot.obj")[0];
		cube  = Mesh::loadFromWavefrontFile("cube.obj")[0];
		floor = Mesh::loadFromWavefrontFile("floor.obj")[0];
		pole = Mesh::loadFromWavefrontFile("pole.obj")[0];
		sphere = Mesh::loadFromWavefrontFile("sphere.obj")[0];
		eye = Mesh::loadFromWavefrontFile("eye.obj")[0];
		quad = Mesh::loadFromWavefrontFile("quad.obj")[0];
		tv = Mesh::loadFromWavefrontFile("tv.obj")[0];

		texSteel = Texture::loadFromFile("steel.png", 8);
		texRust = Texture::loadFromFile("rust.png", 8);
		texEye = Texture::loadFromFile("eye.png", 8);
		texConcrete = Texture::loadFromFile("concrete.png", 8);
		texBuilding = Texture::loadFromFile("building.png", 8);
		texBox = Texture::loadFromFile("box.png", 1);
		texRock = Texture::loadFromFile("rock.png", 8);
		basicProg.use();
		basicProg.setInt("texMain", 0);

		// Créer une texture de rendu comme une texture normale.
		texRender.size = {1024, 768};
		texRender.numLevels = 1;
		glGenTextures(1, &texRender.id);
		glBindTexture(GL_TEXTURE_2D, texRender.id);
		// Créer un tampon de trame (frame buffer);
		glGenFramebuffers(1, &camFrameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, camFrameBuffer);
		// Formatter les données de la texture de rendu en RGBA (RGB aurait aussi été suffisant). Comme avec les VBO de sortie, on peut passer nullptr comme données ou passer une image de départ pour le débogage.
		texRender.setPixelData(GL_RGBA, nullptr);
		// Configurer la texture pour les filtres usuels et pas de répétition.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// Créer un tampon de rendu qui servira de tampon de profondeur. En effet, avec un framebuffer un z-buffer dédié doit être créé.
		glGenRenderbuffers(1, &camZBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, camZBuffer);
		// Définir ce tampon comme tampon de profondeur.
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, camZBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, texRender.size.x, texRender.size.y);
		// Lier le framebuffer à la texture de rendu.
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texRender.id, 0);

		// Appliquer la caméra et perspective habituelle.
		camera.updateProgram(basicProg, view);
		applyPerspective(50, getWindowAspect());
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		basicProg.use();

		// Calculer l'angle de la caméra de surveillance selon le temps écoulé depuis la dernière trame.
		if (not scanPaused) {
			scanValue += getFrameDeltaTime();
			scanValue = fmodf(scanValue, 10.0f);
			scanAngle = 40 * sin(scanValue / 10.0f * 2 * std::numbers::pi_v<float>);
		}
		teapotValue += getFrameDeltaTime();
		teapotValue = fmodf(teapotValue, 5.0f);

		// 1. Dessiner la scène selon le point de vue de la caméra de surveillance dans le framebuffer.
		// 2. Dessiner la scène normalement avec la caméra orbitale principale dans le tampon de la fenêtre.

		// Lier le framebuffer de la caméra secondaire.
		glBindFramebuffer(GL_FRAMEBUFFER, camFrameBuffer);
		// Il faut faire le glClear() pour chaque buffer, car il s'applique sur le buffer de trame actuel, qui est lié par glBindFramebuffer.
		// On peut choisir une couleur de fond différente pour le rendu de la caméra secondaire.
		glClearColor(0.2f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Lire le viewport actuel pour le restaurer plus tard.
		struct { GLint x, y, width, height; } viewport = {};
		glGetIntegerv(GL_VIEWPORT, (GLint*)&viewport);
		// Établir un viewport qui a les mêmes dimensions que la texture de rendu.
		glViewport(0, 0, texRender.size.x, texRender.size.y);
		// Positionner la caméra synthétique juste devant l'oeil. On se rappelle qu'il faut faire l'inverse des opérations quand on bouge la caméra synthétique à travers la matrice de visualisation.
		view.pushIdentity(); {
			view.translate({0, 0, 1.2});
			view.rotate(180, {0, 1, 0});
			view.rotate(-20, {1, 0, 0});
			view.rotate(-scanAngle, {0, 1, 0});
			view.translate({0, -6, 10});
			basicProg.setMat(view);
		} view.pop();
		// Appliquer une perpective pour la caméra secondaire encore là selon les dimensions de la texture de rendu (dans notre cas 1024x768, donc aspect 4:3).
		projection.push(); {
			applyPerspective(40, (float)texRender.size.x / texRender.size.y);
		} projection.pop();
		// Dessiner la scène normalement.
		drawScene();

		// Restaurer la caméra normale.
		basicProg.setMat(view);
		// Restaurer le viewport.
		glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
		// Restaurer la perspective habituelle.
		basicProg.setMat(projection);
		// Délier le framebuffer, donc utiliser le framebuffer de base qui est celui de la fenêtre.
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// Faire le glClear avec le turquoise foncé qu'on utilise depuis le début de la session.
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Dessiner la scène normalement.
		drawScene();
	}

	// Appelée lorsque la fenêtre se ferme.
	void onClose() override {
		for (auto mesh : {&floor, &teapot, &cube, &pole, &sphere, &eye, &quad, &tv})
			mesh->deleteObjects();
		for (auto tex : {&texSteel, &texRust, &texEye, &texConcrete, &texBox, &texBuilding, &texRock, &texRender})
			tex->deleteObject();
		glDeleteFramebuffers(1, &camFrameBuffer);
		glDeleteRenderbuffers(1, &camZBuffer);
		basicProg.deleteShaders();
		basicProg.deleteProgram();
	}

	// Appelée lors d'une touche de clavier.
	void onKeyPress(const sf::Event::KeyEvent& key) override {
		// La touche R réinitialise la position de la caméra.
		// Les touches + et - rapprochent et éloignent la caméra orbitale.
		// Les touches haut/bas change l'élévation ou la latitude de la caméra orbitale.
		// Les touches gauche/droite change la longitude ou le roulement (avec shift) de la caméra orbitale.
		// Espace met en pause le mouvement de la caméra de surveillance.

		camera.handleKeyEvent(key, 5, 0.5, {10, 15, 30, 0, {0, 2, -5}});
		camera.updateProgram(basicProg, view);

		using enum sf::Keyboard::Key;
		switch (key.code) {
		case Space:
			scanPaused ^= 1;
			std::cout << "Scan " << (scanPaused ? "pause" : "unpause") << "\n";
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
		camera.updateProgram(basicProg, view);
	}

	// Appelée lors d'un défilement de souris.
	void onMouseScroll(const sf::Event::MouseWheelScrollEvent& mouseScroll) override {
		// Zoom in/out
		camera.altitude -= mouseScroll.delta;
		camera.updateProgram(basicProg, view);
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::SizeEvent& event) override {
		applyPerspective(50, getWindowAspect());
	}

	void drawScene() {
		// Il ne se passe rien de spécial ici. En effet, une fois que le framebuffer et la texture de rendu sont configurée, on dessine la scène comme si de rien était, pas besoin d'un nuanceur différent (quoiqu'on pourrait choisir d'en utiliser un différent pour faire des effets). La seule petite particularité est la texture utilisée pour l'écran de la TV qui est en fait la texture de rendu.

		basicProg.use();

		model.loadIdentity();
		basicProg.setMat(model);

		// Le plancher en ciment.
		model.push(); {
			model.translate({0, 0, -0.5});
			basicProg.setMat(model);
		} model.pop();
		texConcrete.bindToTextureUnit(0);
		floor.draw();

		// Le cube qui ressemble à un bâtiment.
		model.push(); {
			model.translate({-5, 1.45, 5});
			model.scale({1, 1.5, 1});
			basicProg.setMat(model);
		} model.pop();
		texBuilding.bindToTextureUnit(0);
		cube.draw();

		// La grosse boîte de carton.
		model.push(); {
			model.translate({4, 1.45, 4});
			model.rotate(180, {0, 1, 0});
			model.scale({1.5, 1.5, 1.5});
			basicProg.setMat(model);
		} model.pop();
		texBox.bindToTextureUnit(0);
		cube.draw();

		// Le pole de rotation de la théière.
		model.push(); {
			model.translate({-3.8, 2.8, 4.5});
			model.rotate(90, {1, 0, 0});
			model.scale({0.5, 0.2, 0.5});
			basicProg.setMat(model);
		} model.pop();
		texRust.bindToTextureUnit(0);
		pole.draw();

		// La théière qui bouge.
		model.push(); {
			float angle = 20 * sin(teapotValue / 5 * 2 * std::numbers::pi_v<float>);
			model.translate({-3.7, 2.9, 5});
			model.rotate(90, {0, 1, 0});
			model.rotate(angle, {1, 0, 0});
			model.translate({0, -0.3, 1.5});
			basicProg.setMat(model);
		} model.pop();
		texRock.bindToTextureUnit(0);
		teapot.draw();

		// Le poteau au bout duquel se trouve l'oeil observateur.
		model.push(); {
			model.translate({0, 0, -10});
			model.scale({0.75, 1, 0.75});
			basicProg.setMat(model);
		} model.pop();
		texRust.bindToTextureUnit(0);
		pole.draw();

		// La sphère autour de laquelle l'oeil tourne.
		model.push(); {
			model.translate({0, 6, -10});
			basicProg.setMat(model);
		} model.pop();
		texRust.bindToTextureUnit(0);
		sphere.draw();

		// L'oeil observateur. On applique l'angle qui change dans le temps.
		model.push(); {
			model.translate({0, 6, -10});
			model.rotate(scanAngle, {0, 1, 0});
			model.rotate(20, {1, 0, 0});
			basicProg.setMat(model);
		} model.pop();
		texEye.bindToTextureUnit(0);
		eye.draw();

		// La TV et l'écran (le quad texturé à l'intérieur de la TV). Ils ont le même positionnement et mise à l'échelle, donc même matrice de modélisation.
		model.push(); {
			model.translate({0, 2.5, -9.45});
			model.scale({(float)texRender.size.x / texRender.size.y, 1, 1});
			model.scale({2, 2, 1});
			basicProg.setMat(model);
		} model.pop();
		texSteel.bindToTextureUnit(0);
		tv.draw();
		// Ici on utilise la texture de rendu comme texture de l'objet. C'est tout ça le but d'utiliser une texture comme sortie du framebuffer, pas besoin de lire vers le CPU pour ensuite renvoyer sur le GPU; tout reste dans la mémoire graphique.
		texRender.bindToTextureUnit(0);
		quad.draw();
	}

	void applyPerspective(float fovy, float aspect) {
		projection.perspective(fovy, aspect, 0.1f, 100.0f);
		basicProg.setMat(projection);
	}

	void loadShaders() {
		// On utilise les nuanceur de base qui échantillonent les textures, pas besoin de quoique ce soit de fancy.
		basicProg.create();
		basicProg.attachSourceFile(GL_VERTEX_SHADER, "basic_vert.glsl");
		basicProg.attachSourceFile(GL_FRAGMENT_SHADER, "basic_frag.glsl");
		basicProg.link();
	}
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	settings.context.antialiasingLevel = 4;

	App app;
	app.run(argc, argv, "Exemple Semaine 9: Rétroaction avec texture de rendu", settings);
}
