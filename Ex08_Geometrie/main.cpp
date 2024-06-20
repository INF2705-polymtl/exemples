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
#include <inf2705/SceneObject.hpp>
#include <inf2705/OrbitCamera.hpp>


using namespace gl;
using namespace glm;


// Un spritesheet est une vielle méthode d'animation numérique. C'est à la base une image contient plusieurs sprites (lutins). Ces sprites sont de petites images qui, ensemble, forment les trames d'une animation. Chaque sprite dans le spritesheet est placé dans une disposition régulière (horizontalement dans notre cas). On utilise ensuite un index dans le spritesheet pour afficher le sprite correct à l'écran.
struct SpriteSheet
{
	sf::Image originalImg;
	std::vector<Texture> sprites;
	ivec2 elemSize;

	void bindToTextureUnit(int spriteIndex, int activeUnit) {
		sprites[spriteIndex].bindToTextureUnit(activeUnit);
	}

	static SpriteSheet loadFromFile(const std::string& filename, ivec2 spriteElemSize, int numSprites) {
		SpriteSheet result;
		// Charger l'image avec tous les éléments.
		if (not result.originalImg.loadFromFile(filename)) {
			std::cerr << std::format("{} could not be loaded", filename) << "\n";
			return {};
		}

		result.elemSize = spriteElemSize;
		sf::Image spriteImg;
		spriteImg.create(spriteElemSize.x, spriteElemSize.y);
		// Pour chaque sprite :
		for (int i = 0; i < numSprites; i++) {
			// Copier la partie de l'image originale.
			spriteImg.copy(
				result.originalImg,
				0, 0,
				{i * spriteElemSize.x, 0, spriteElemSize.x, spriteElemSize.y}
			);
			// Charger cette sous-image dans une texture séparée.
			result.sprites.push_back(Texture::loadFromImage(spriteImg));
			// Configurer la texture pour ne pas appliquer de filtre. C'est une texture qui est supposée être pixelisée.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		return result;
	}
};

struct App : public OpenGLApplication
{
	Mesh point;
	Mesh line;
	Mesh d20;
	Texture texRust;
	SpriteSheet spriteLink;
	SpriteSheet spriteSword;

	ShaderProgram uniColorProg;
	ShaderProgram extrudeSpikesProg;
	ShaderProgram spritesProg;

	TransformStack modelExtrude = {"model"};
	TransformStack modelSprite = {"model"};
	TransformStack view = {"view"};
	TransformStack projection = {"projection"};

	OrbitCamera camera = {5, 30, 30, 0};

	int swingStartFrame = -1;
	int swordStartFrame = -1;
	Uniform<float> extrudeLength = {"extrudeLength", 0.0f};
	Uniform<bool> usingWorldPositions = {"usingWorldPositions", false};

	// Appelée avant la première trame.
	void init() override {
		setKeybindMessage(
			"R : réinitialiser la position de la caméra." "\n"
			"+ et - :  rapprocher et éloigner la caméra orbitale." "\n"
			"haut/bas : changer la latitude de la caméra orbitale." "\n"
			"gauche/droite : changer la longitude ou le roulement (avec shift) de la caméra orbitale." "\n"
			"clic central (cliquer la roulette) : bouger la caméra en glissant la souris." "\n"
			"roulette : rapprocher et éloigner la caméra orbitale." "\n"
			"U : Augmenter l'extrusion." "\n"
			"I : Diminuer l'extrusion." "\n"
			"O : Appliquer l'extrusion sur les coordonnées de scène ou d'objet." "\n"
			"W et S : Étirer/compresser le d20 en Y." "\n"
			"A et D : Étirer/compresser le d20 en XZ." "\n"
			"Espace : Coup d'épée (début d'animation)." "\n"
		);

		// Config de base, lignes assez visibles.
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_POINT_SMOOTH);
		glPointSize(3.0f);
		glLineWidth(10.0f);
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);

		loadShaders();

		// Le d20 qui sera extrudé.
		d20 = Mesh::loadFromWavefrontFile("d20.obj")[0];
		// La ligne séparant les deux viewports.
		line.vertices = {
			{{-1, 0, 0}, {}, {}},
			{{ 1, 0, 0}, {}, {}},
		};
		line.setup();
		// Un point centré à l'origine. Il est utilisé pour afficher les lutins (sprites) qui ne nécessitent qu'un seul sommet.
		point.vertices = {
			{{0, 0, 0}, {}, {}}
		};
		point.setup();

		// Charger la texture rouillée du D20.
		texRust = Texture::loadFromFile("rust.png", 4);
		// Charger les spritesheets du personnnage et de l'épée en spécifiant la taille d'un élément dans la feuille et du nombre d'éléments à découper
		spriteLink = SpriteSheet::loadFromFile(
			"link_spritesheet.png",
			{27, 16},
			4
		);
		spriteSword = SpriteSheet::loadFromFile(
			"sword_spritesheet.png",
			{16, 7},
			4
		);

		// Les liaisons et variables uniformes constantes.
		texRust.bindToTextureUnit(0);
		extrudeSpikesProg.use();
		extrudeSpikesProg.setInt("texMain", 0);
		spritesProg.use();
		spritesProg.setInt("texMain", 1);
		uniColorProg.use();
		uniColorProg.setVec("globalColor", vec4(1, 0.7f, 0.7f, 1));

		camera.updateProgram(extrudeSpikesProg, view);
		applyPerspective();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		auto winSize = getWindow().getSize();

		// Viewport de la moitié supérieur de la fenêtre.
		glViewport(0, winSize.y / 2, winSize.x, winSize.y / 2);
		drawExtrudedD20();

		glDisable(GL_DEPTH_TEST);

		// Viewport de la moitié inférieur de la fenêtre.
		glViewport(0, 0, winSize.x, winSize.y / 2);
		drawAnimatedCharacter();
		drawAnimatedSword();

		// Viewport de la fenêtre au complet
		glViewport(0, 0, winSize.x, winSize.y);
		// Dessiner une ligne pour séparer visuellement la fenêtre en deux.
		uniColorProg.use();
		line.draw(GL_LINES);

		glEnable(GL_DEPTH_TEST);
	}

	// Appelée lors d'une touche de clavier.
	void onKeyPress(const sf::Event::KeyEvent& key) override {
		// La touche R réinitialise la position de la caméra.
		// Les touches + et - rapprochent et éloignent la caméra orbitale.
		// Les touches haut/bas change l'élévation ou la latitude de la caméra orbitale.
		// Les touches gauche/droite change la longitude ou le roulement (avec shift) de la caméra orbitale.
		//
		// U : Augmenter l'extrusion.
		// I : Diminuer l'extrusion.
		// O : Appliquer l'extrusion sur les coordonnées de scène ou d'objet.
		// W et S : Étirer/compresser le d20 en Y
		// A et D : Étirer/compresser le d20 en XZ
		// Espace : Coup d'épée (début d'animation).

		camera.handleKeyEvent(key, 5, 0.5, {5, 30, 30, 0});
		camera.updateProgram(extrudeSpikesProg, view);

		using enum sf::Keyboard::Key;
		switch (key.code) {
		case U:
			extrudeLength = std::max(0.0f, extrudeLength + 0.1f);
			break;
		case I:
			extrudeLength = std::max(0.0f, extrudeLength - 0.1f);
			break;
		case O:
			usingWorldPositions ^= 1;
			break;

		case A:
			modelExtrude.scale({1.1f, 1, 1.1f});
			break;
		case D:
			modelExtrude.scale({0.9f, 1, 0.9f});
			break;
		case W:
			modelExtrude.scale({1, 1.1f, 1});
			break;
		case S:
			modelExtrude.scale({1, 0.9f, 1});
			break;

		case Space:
			swingStartFrame = swordStartFrame = getCurrentFrameNumber();
			break;
		}

		// Mettre à jour les variables uniformes.
		extrudeSpikesProg.setUniform(extrudeLength);
		extrudeSpikesProg.setUniform(usingWorldPositions);
	}

	// Appelée lors d'un mouvement de souris.
	void onMouseMove(const sf::Event::MouseMoveEvent& mouseDelta) override {
		// Mettre à jour la caméra si on a un clic de la roulette.
		auto& mouse = getMouse();
		camera.handleMouseMoveEvent(mouseDelta, mouse, deltaTime_ / (0.7f / 30));
		camera.updateProgram(extrudeSpikesProg, view);
	}

	// Appelée lors d'un défilement de souris.
	void onMouseScroll(const sf::Event::MouseWheelScrollEvent& mouseScroll) override {
		// Zoom in/out
		camera.altitude -= mouseScroll.delta;
		camera.updateProgram(extrudeSpikesProg, view);
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::SizeEvent& event) override {
		applyPerspective();
	}

	void drawExtrudedD20() {
		// Appliquer les transformations et afficher.
		extrudeSpikesProg.use();
		extrudeSpikesProg.setMat(modelExtrude);
		d20.draw();
	}

	void drawAnimatedCharacter() {
		spritesProg.use();

		// Calculer l'état de l'animation.
		int numSwingAnimFrames = (int)spriteLink.sprites.size();
		int swingAnimFrame = 0;
		if (swingStartFrame != -1) {
			swingAnimFrame = getCurrentFrameNumber() - swingStartFrame;
			swingAnimFrame = std::clamp(swingAnimFrame / 4, 0, numSwingAnimFrames);
		}

		// Choisir quel lutin (sprite) utiliser selon la trame actuelle.
		spriteLink.bindToTextureUnit(swingAnimFrame % numSwingAnimFrames, 1);

		modelSprite.loadIdentity();
		// Positionner le point.
		modelSprite.translate({-0.6, 0, 0});
		// Mettre à l'échelle en appliquant les proportions de la texture.
		modelSprite.scale({spriteLink.elemSize.x * 0.1f, spriteLink.elemSize.y * 0.1f, 1});
		modelSprite.scale({0.1, 0.1, 1});

		spritesProg.setMat(modelSprite);
		point.draw(GL_POINTS);
	}

	void drawAnimatedSword() {
		spritesProg.use();

		// Calculer l'état de l'animation.
		int linkAnimLength = (int)spriteLink.sprites.size() * 4;
		int swordAnimFrame = getCurrentFrameNumber() - swingStartFrame - linkAnimLength;
		if (swingStartFrame == -1 or swordAnimFrame < 0)
			return;
		int numSwordAnimFrames = (int)spriteSword.sprites.size();

		// Choisir quel lutin (sprite) utiliser selon la trame actuelle.
		spriteSword.bindToTextureUnit(swordAnimFrame / 2 % numSwordAnimFrames, 1);

		modelSprite.loadIdentity();
		// Positionner le point en fonction de la trame actuelle (donc mouvement vers la droite).
		modelSprite.translate({-0.5 + swordAnimFrame * 0.05, -0.015, 0});
		// Mettre à l'échelle en appliquant les proportions de la texture.
		modelSprite.scale({spriteSword.elemSize.x * 0.1f, spriteSword.elemSize.y * 0.1f, 1});
		modelSprite.scale({0.1, 0.1, 1});
		spritesProg.setMat(modelSprite);
		point.draw(GL_POINTS);
	}

	void applyPerspective(float fovy = 50) {
		float viewportAspect = getWindowAspect() * 2;

		// Appliquer la perspective avec un champs de vision (FOV) vertical donné et avec un aspect correspondant à celui de la fenêtre. C'est pour la forme extrudée dans le haut de la fenêtre.
		projection.perspective(fovy, viewportAspect, 0.01f, 100.0f);
		extrudeSpikesProg.use();
		extrudeSpikesProg.setMat(projection);

		// Appliquer une projection orthogonale en respectant l'aspect de la fenêtre. C'est pour les lutins dans le bas de la fenêtre.
		projection.ortho(-1, 1, -1.0f / viewportAspect, 1.0f / viewportAspect, -1, 1);
		spritesProg.use();
		spritesProg.setMat(projection);
	}

	void loadShaders() {
		uniColorProg.attachSourceFile(GL_VERTEX_SHADER, "basic_vert.glsl");
		uniColorProg.attachSourceFile(GL_FRAGMENT_SHADER, "uniform_frag.glsl");
		uniColorProg.link();

		// Pour les deux programmes utilisant des textures (extrusions et sprites), on peut utiliser le nuanceur de fragments de base qui échantillonne simplement une texture avec des coordonnées en entrée.
		extrudeSpikesProg.attachSourceFile(GL_VERTEX_SHADER, "extrude_vert.glsl");
		extrudeSpikesProg.attachSourceFile(GL_GEOMETRY_SHADER, "extrude_geom.glsl");
		extrudeSpikesProg.attachSourceFile(GL_FRAGMENT_SHADER, "basic_frag.glsl");
		extrudeSpikesProg.link();

		// On peut réutiliser le nuanceur de sommets du programme d'extrusion pour avoir les positions originales passées en sortie.
		spritesProg.attachSourceFile(GL_VERTEX_SHADER, "extrude_vert.glsl");
		spritesProg.attachSourceFile(GL_GEOMETRY_SHADER, "sprites_geom.glsl");
		spritesProg.attachSourceFile(GL_FRAGMENT_SHADER, "basic_frag.glsl");
		spritesProg.link();
	}
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.videoMode = {600, 600};
	settings.fps = 30;
	settings.context.antialiasingLevel = 4;

	App app;
	app.run(argc, argv, "Exemple Semaine 8: Nuanceurs de géométrie", settings);
}
