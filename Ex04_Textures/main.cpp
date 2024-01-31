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
#include <SFML/Graphics.hpp>

#include <inf2705/OpenGLApplication.hpp>
#include <inf2705/Mesh.hpp>
#include <inf2705/TransformStack.hpp>
#include <inf2705/ShaderProgram.hpp>
#include <inf2705/OrbitCamera.hpp>


using namespace gl;
using namespace glm;


struct App : public OpenGLApplication
{
	// Appelée avant la première trame.
	void init() override {
		// Config de base: pas de cull, lignes assez visibles.
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_POINT_SMOOTH);
		glPointSize(3.0f);
		glLineWidth(3.0f);
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);

		GLint maxTexUnits = 0;
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTexUnits);
		std::cout << "Max texture units: " << maxTexUnits << "\n";

		loadShaders();

		updateCamera();
		applyPerspective();

		cubeBox = Mesh::loadFromWavefrontFile("cube_box.obj")[0];
		cubeBox.setup();

		cubeRoad = Mesh::loadFromWavefrontFile("cube_road.obj")[0];
		cubeRoad.setup();

		loadTextures();

		bindBoxTextures();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		switch (mode) {
		case 1:
			progCompositing.use();
			progCompositing.setMat("model", model);
			cubeBox.draw();
			break;
		case 2:
		case 3:
			progBasic.use();
			progBasic.setMat("model", model);
			cubeRoad.draw();
			break;
		}
	}

	// Appelée lors d'une touche de clavier.
	void onKeyEvent(const sf::Event::KeyEvent& key) override {
		using enum sf::Keyboard::Key;
		switch (key.code) {
		// Réinitialiser la position de la caméra.
		case R:
			camera = {};
			break;

		// Les touches + et - servent à rapprocher et éloigner la caméra orbitale.
		case Add:
			camera.altitude -= 0.5f;
			break;
		case Subtract:
			camera.altitude += 0.5f;
			break;

		// Les touches haut/bas change l'élévation ou la latitude de la caméra orbitale.
		case Up:
			camera.moveNorth(5.0f);
			break;
		case Down:
			camera.moveSouth(5.0f);
			break;

		// Les touches gauche/droite change la longitude ou le roulement (avec shift) de la caméra orbitale.
		case Left:
			if (key.shift)
				camera.rollCCW(5.0f);
			else
				camera.moveWest(5.0f);
			break;
		case Right:
			if (key.shift)
				camera.rollCW(5.0f);
			else
				camera.moveEast(5.0f);
			break;

		// Touches numériques: changer l'exemple courant.
		case Num1: // 1: Boîte en carton avec du texte compositionné.
			bindBoxTextures();
			mode = 1;
			break;
		case Num2: // 2: Exemple de route avec texture qui se répète.
			progBasic.use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texAsphalt);
			progBasic.setTextureUnit("tex0", 0);
			mode = 2;
			break;
		case Num3: // 3: Exemple de Mipmap manuel.
			progBasic.use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texLevels);
			progBasic.setTextureUnit("tex0", 0);
			mode = 3;
			break;
		}

		updateCamera();
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::SizeEvent& event) override {
		// Mettre à jour la matrice de projection avec le nouvel aspect de fenêtre après le redimensionnement.
		applyPerspective();
	}

	void loadShaders() {
		progBasic.create();
		progBasic.attachSourceFile(GL_VERTEX_SHADER, "basic_vert.glsl");
		progBasic.attachSourceFile(GL_FRAGMENT_SHADER, "basic_frag.glsl");
		progBasic.link();

		progCompositing.create();
		progCompositing.attachSourceFile(GL_VERTEX_SHADER, "basic_vert.glsl");
		progCompositing.attachSourceFile(GL_FRAGMENT_SHADER, "composite_frag.glsl");
		progCompositing.link();
	}

	void loadTextures() {
		texBlank = loadTextureFromFile("blank.png", false);
		texBoxBG = loadTextureFromFile("box_bg.png", true);
		texBoxText = loadTextureFromFile("box_text.png", true);

		texAsphalt = loadTextureFromFile("asphalt.png", true);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		texLevels = loadMipmapFromFiles("lvl{}.png", 6);

	}

	GLuint loadTextureFromFile(const std::string& filename, bool generateMipmap = true) {
		sf::Image texImg;
		texImg.loadFromFile(filename);
		// Beaucoup de bibliothèques importent les images avec x=0,y=0 (donc premier pixel du tableau) au coin haut-gauche de l'image. C'est la convention en graphisme, mais les textures en OpenGL ont leur origine au coin bas-gauche.
		// SFML applique la convention origine = haut-gauche, il faut donc renverser l'image verticalement avant de la passer à OpenGL.
		texImg.flipVertically();
		GLuint texID = 0;
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA,
			texImg.getSize().x,
			texImg.getSize().y,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			texImg.getPixelsPtr()
		);

		if (generateMipmap) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glGenerateMipmap(GL_TEXTURE_2D);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		return texID;
	}

	GLuint loadMipmapFromFiles(const std::string& filenameFormat, int numLevels) {
		GLuint texID = 0;
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);
		for (int i = 0; i < numLevels; i++) {
			auto filename = std::vformat(filenameFormat, std::make_format_args(i));
			sf::Image texImg;
			texImg.loadFromFile(filename);
			texImg.flipVertically();
			glTexImage2D(
				GL_TEXTURE_2D,
				i,
				GL_RGBA,
				texImg.getSize().x,
				texImg.getSize().y,
				0,
				GL_RGBA,
				GL_UNSIGNED_BYTE,
				texImg.getPixelsPtr()
			);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numLevels - 1);

		return texID;
	}

	void bindBoxTextures() {
		progCompositing.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texBoxBG);
		progCompositing.setTextureUnit("tex0", 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texBoxText);
		progCompositing.setTextureUnit("tex1", 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, texBlank);
		progCompositing.setTextureUnit("tex2", 2);
	}

	void updateCamera() {
		view.pushIdentity();
		camera.applyView(view);
		// En positionnant la caméra, on met seulement à jour la matrice de visualisation.
		setMatrixOnAll("view", view);
		view.pop();
	}

	void applyPerspective() {
		// Calculer l'aspect de notre caméra à partir des dimensions de la fenêtre.
		auto windowSize = window_.getSize();
		float aspect = (float)windowSize.x / windowSize.y;

		projection.pushIdentity();
		// Appliquer la perspective avec un champs de vision (FOV) vertical donné et avec un aspect correspondant à celui de la fenêtre.
		projection.perspective(50, aspect, 0.01f, 100.0f);
		setMatrixOnAll("projection", projection);
		projection.pop();
	}

	void setMatrixOnAll(std::string_view name, const TransformStack& matrix) {
		// Les programmes de nuanceur ont leur propre espace mémoire. Il faut donc mettre à jour les variables uniformes pour chaque programme.
		for (auto&& prog : allPrograms) {
			prog.use();
			prog.setMat(name, matrix);
		}
	}

	Mesh pyramid;
	Mesh cubeBox;
	Mesh cubeRoad;
	ShaderProgram allPrograms[2] = {};
	ShaderProgram& progBasic = allPrograms[0];
	ShaderProgram& progCompositing = allPrograms[1];

	GLuint texBlank = 0;
	GLuint texBoxBG = 0;
	GLuint texBoxText = 0;
	GLuint texAsphalt = 0;
	GLuint texLevels = 0;

	TransformStack model;
	TransformStack view;
	TransformStack projection;

	OrbitCamera camera = {5, 30, -30, 0};

	int mode = 1;
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	settings.context.antialiasingLevel = 0;

	App app;
	app.run(argc, argv, "Exemple Semaine 4: Textures", settings);
}
