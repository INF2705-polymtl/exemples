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
	Mesh cubeBox;
	Mesh cubeRoad;
	Mesh quad;
	ShaderProgram allPrograms[2] = {};
	ShaderProgram& progBasic = allPrograms[0];
	ShaderProgram& progCompositing = allPrograms[1];

	GLuint texBlank = 0;
	GLuint texBoxBG = 0;
	GLuint texBoxText = 0;
	GLuint texAsphalt = 0;
	GLuint texLevels = 0;
	GLuint texTest = 0;

	TransformStack model;
	TransformStack view;
	TransformStack projection;

	OrbitCamera camera = {5, 30, -30, 0};

	int mode = 1;

	// Appelée avant la première trame.
	void init() override {
		setKeybindMessage(
			"F5 : Capture d'écran" "\n"
			"R : réinitialiser la position de la caméra." "\n"
			"+ et - :  rapprocher et éloigner la caméra orbitale." "\n"
			"haut/bas : changer la latitude de la caméra orbitale." "\n"
			"gauche/droite : changer la longitude ou le roulement (avec shift) de la caméra orbitale." "\n"
			"clic droit ou central : bouger la caméra en glissant la souris." "\n"
			"roulette : rapprocher et éloigner la caméra orbitale." "\n"
			"1 : Boîte en carton avec du texte compositionné." "\n"
			"2 : Exemple de route avec texture qui se répète." "\n"
			"3 : Exemple de Mipmap manuel." "\n"
			"4 : Démo des modes de débordement." "\n"
		);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);

		// On peut demander le nombre maximal d'unités de texture (les glActiveTexture). Le standard demande au moins 80.
		GLint maxTexUnits = 0;
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTexUnits);
		std::cout << "Max texture units: " << maxTexUnits << "\n";

		loadShaders();

		// Caméra orbitale, perspective pas trop large.
		updateCamera();
		applyPerspective();

		// Pour le reste de la session, ça va devenir de moins en moins faisable de hard-coder toutes les données à la main. On va plutôt importer nos mesh à partir de fichiers Wavefront (.obj). Ceux-ci contiennent les positions, normales et coordonnées de texture.
		// Beaucoup de logiciels de modélisation 3D (Blender, 3ds Max, même Wings 3D) supporte l'exportation en Wavefront.
		cubeBox = Mesh::loadFromWavefrontFile("cube_box.obj")[0];
		cubeRoad = Mesh::loadFromWavefrontFile("cube_road.obj")[0];
		// On va faire quand même coder à la main les cas simples.
		quad.vertices = {
			{{-1, -1, 0}, {}, {-1, -1}},
			{{ 1, -1, 0}, {}, { 2, -1}},
			{{ 1,  1, 0}, {}, { 2,  2}},
			{{-1,  1, 0}, {}, {-1,  2}}
		};
		quad.indices = {
			0, 1, 2,
			0, 2, 3
		};
		quad.setup();

		// Charger les textures. On peut expérimenter avec la génération automatique de mipmaps (deuxième paramètre de la fonction).
		texBlank = loadTextureFromFile("blank.png", false);
		texBoxBG = loadTextureFromFile("box_bg.png", true);
		texBoxText = loadTextureFromFile("box_text.png", false);
		texTest = loadTextureFromFile("test.png", false);

		texAsphalt = loadTextureFromFile("asphalt.png", false);
		// Ici, vous pouvez expérimenter avec le mode de dépassement des coordonnées de textures.
		// Les valeurs possibles sont GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, ou GL_MIRROR_CLAMP_TO_EDGE.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// Le "{}" dans le nom du fichier sera remplacé par le numéro de détail (on a donc "lvl0.png", "lvl1.png", ..., "lvl5.png").
		texLevels = loadMipmapFromFiles("lvl{}.png", 6);

		// Affecter l'unité 0 à la variable uniforme tex0 et ainsi de suite. Le sampler avec la valeur 0 lit de l'unité de texture GL_TEXTURE0.
		for (auto& prog : allPrograms) {
			prog.use();
			prog.setInt("tex0", 0);
			prog.setInt("tex1", 1);
			prog.setInt("tex2", 2);
		}
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		switch (mode) {
		case 1:
			// Pour la boîte en carton, on superpose des textures.
			progCompositing.use();

			// Lier la texture de carton à l'unité de texture 0.
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texBoxBG);
			// Lier la texture de texte à l'unité de texture 1.
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, texBoxText);
			// Lier la texture transparente à l'unité de texture 2.
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, texBlank);

			model.push(); {
				progCompositing.setMat("model", model);
			} model.pop();
			cubeBox.draw();
			break;
		case 2:
		case 3:
			// Pour les autres modes, on applique simplement une texture.
			progBasic.use();
			// Lier la texture d'asphalte à l'unité de texture 0.
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mode == 2 ? texAsphalt : texLevels);
			model.push(); {
				progCompositing.setMat("model", model);
			} model.pop();
			cubeRoad.draw();
			break;
		case 4:
			progBasic.use();

			// Lier la texture de test à l'unité de texture 0.
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texTest);

			// Appliquer une projection orthogonale et une caméra fixe.
			projection.pushIdentity();
			float aspect = getWindowAspect();
			projection.ortho(-4 * aspect, 4 * aspect, -4, 4, -1, 1);
			progBasic.setMat("projection", projection);
			progBasic.setMat("view", mat4(1));

			model.pushIdentity();

			// Haut-gauche
			model.translate({-2.5, 2.5, 0});
			progBasic.setMat("model", model);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			quad.draw();

			// Haut-centre
			model.translate({2.5, 0, 0});
			progBasic.setMat("model", model);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
			quad.draw();

			// Haut-droite
			model.translate({2.5, 0, 0});
			progBasic.setMat("model", model);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			quad.draw();

			// Centre-gauche
			model.translate({-5, -2.5, 0});
			progBasic.setMat("model", model);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			quad.draw();

			// Centre
			model.translate({2.5, 0, 0});
			progBasic.setMat("model", model);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			quad.draw();

			// Centre-droite
			model.translate({2.5, 0, 0});
			progBasic.setMat("model", model);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
			quad.draw();

			// Bas-gauche
			model.translate({-5, -2.5, 0});
			progBasic.setMat("model", model);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			quad.draw();

			// Bas-centre
			model.translate({2.5, 0, 0});
			progBasic.setMat("model", model);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
			quad.draw();

			// Bas-droite
			model.translate({2.5, 0, 0});
			progBasic.setMat("model", model);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			quad.draw();

			model.pop();
			projection.pop();
			progBasic.setMat("model", model);
			progBasic.setMat("view", view);
			progBasic.setMat("projection", projection);

			break;
		}
	}

	// Appelée lorsque la fenêtre se ferme.
	void onClose() override {
		cubeBox.deleteObjects();
		cubeRoad.deleteObjects();
		quad.deleteObjects();
		glDeleteTextures(1, &texBlank);
		glDeleteTextures(1, &texBoxBG);
		glDeleteTextures(1, &texBoxText);
		glDeleteTextures(1, &texAsphalt);
		glDeleteTextures(1, &texLevels);
		glDeleteTextures(1, &texTest);
		texBlank = texBoxBG = texBoxText = texAsphalt = texLevels = texTest = 0;
		progBasic.deleteShaders();
		progBasic.deleteProgram();
		progCompositing.deleteShaders();
		progCompositing.deleteProgram();
	}

	// Appelée lors d'une touche de clavier.
	void onKeyPress(const sf::Event::KeyPressed& key) override {
		// La touche R réinitialise la position de la caméra.
		// Les touches + et - rapprochent et éloignent la caméra orbitale.
		// Les touches haut/bas change l'élévation ou la latitude de la caméra orbitale.
		// Les touches gauche/droite change la longitude ou le roulement (avec shift) de la caméra orbitale.
		camera.handleKeyEvent(key, 5.0f, 0.5f, {5, 30, -30, 0});
		updateCamera();

		using enum sf::Keyboard::Key;
		switch (key.code) {
		// Touches numériques: changer l'exemple courant.
		case Num1: // 1: Boîte en carton avec du texte compositionné.
		case Num2: // 2: Exemple de route avec texture qui se répète.
		case Num3: // 3: Exemple de Mipmap manuel.
		case Num4: // 4: Démonstration des modes de débordement.
			mode = (int)key.code - (int)Num0;
			break;

		case F5: {
			std::string path = saveScreenshot();
			std::cout << "Capture d'écran dans " << path << std::endl;
			break;
		}}
	}

	// Appelée lors d'un mouvement de souris.
	void onMouseMove(const sf::Event::MouseMoved& mouseDelta) override {
		// Mettre à jour la caméra si on a un clic droit ou central.
		auto& mouse = getMouse();
		camera.handleMouseMoveEvent(mouseDelta, mouse, deltaTime_ / (0.7f / 30));
		updateCamera();
	}

	// Appelée lors d'un défilement de souris.
	void onMouseScroll(const sf::Event::MouseWheelScrolled& mouseScroll) override {
		// Zoom in/out
		camera.altitude -= mouseScroll.delta;
		updateCamera();
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::Resized& event) override {
		// Mettre à jour la matrice de projection avec le nouvel aspect de fenêtre après le redimensionnement.
		applyPerspective();
	}

	GLuint loadTextureFromFile(const std::string& filename, bool generateMipmap = true) {
		// Lire les pixels de l'image. SFML (la bibliothèque qu'on utilise pour gérer la fenêtre) a déjà une fonctionnalité de chargement d'images. Une alternative plus légère est stb_image.
		sf::Image texImg;
		bool ok = texImg.loadFromFile(filename);
		// Beaucoup de bibliothèques importent les images avec x=0,y=0 (donc premier pixel du tableau) au coin haut-gauche de l'image. C'est la convention en graphisme, mais les textures en OpenGL ont leur origine au coin bas-gauche.
		// SFML applique la convention origine = haut-gauche, il faut donc renverser l'image verticalement avant de la passer à OpenGL.
		texImg.flipVertically();

		// Générer et lier un objet de texture. Ça ressemble un peu aux VBO.
		GLuint texID = 0;
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);
		// Passer les données de l'image (un peu comme avec glBufferData). Il faut spécifier le format interne qui sera enregistré sur le GPU ainsi que celui dont est fait le tableau de données passé en paramètre.
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

		// Le paramètre contrôle la génération automatique de mipmaps.
		if (generateMipmap) {
			// Spécifier le mode de filtrage pour la minimisation (zoom out). Si on utilise du mipmap, il faut utiliser le mode spécifique aux mipmaps (GL_NEAREST_MIPMAP_* ou GL_LINEAR_MIPMAP_*)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			// Spécifier le mode de filtrage pour le grossissement (zoom in). Beaucoup de ressources en ligne font l'erreur d'utiliser GL_*_MIPMAP_* pour le grossissement. Les seules valeurs applicables sont GL_LINEAR et GL_NEAREST.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// Générer automatiquement les mipmaps. L'algorithme utilisé pour faire la mise à l'échelle n'est pas spécifiée dans le standard OpenGL. C'est un compromis entre la solution simple (pas de mipmap) et la solution compliqué (mipmap manuel).
			glGenerateMipmap(GL_TEXTURE_2D);
		} else {
			// Spéficier les modes de filtrages. GL_NEAREST pour la minimisation et GL_LINEAR pour le grossissement fonctionnent bien dans une majorité des cas.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		return texID;
	}

	GLuint loadMipmapFromFiles(const std::string& filenameFormat, int numLevels) {
		// Créer et lier l'objet de texture. Quand on fait des mipmap manuellement, il faut créer une seule texture à laquelle on passe une image différente pour chaque niveau de détail.
		GLuint texID = 0;
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);
		// Pour chaque niveau de détails:
		for (int i = 0; i < numLevels; i++) {
			// Générer le nom de fichier (du beau C++20).
			auto filename = std::vformat(filenameFormat, std::make_format_args(i));
			// Charger l'image et la renverser verticalement (voir commentaire dans fonction précédente).
			sf::Image texImg;
			texImg.loadFromFile(filename);
			texImg.flipVertically();
			// Passer l'image en spécifiant le niveau de détail (2e paramètre de glTexImage2D).
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

		// Spécifier le mode de filtrage. On utilise les mêmes que si on utilisait glGenerateMipmap.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Spécifier (optionnellement) le niveau de détail correspondant à la définition de base. Par défaut c'est 0 et donc pas nécessaire de le modifier.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		// ATTENTION: Ce n'est pas super clair dans la documentation officielle, mais il faut configurer le GL_TEXTURE_MAX_LEVEL quand on fait des mipmap manuellement. Le défaut est 1000, il s'attend donc à recevoir 1000 tableaux de pixels.
		// Dans notre cas on a 6 niveaux, donc 0 à 5, alors on passe 5.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numLevels - 1);

		return texID;
	}

	void updateCamera() {
		camera.applyToView(view);
		// En positionnant la caméra, on met seulement à jour la matrice de visualisation.
		setMatrixOnAll("view", view);
	}

	void applyPerspective() {
		// Calculer l'aspect de notre caméra à partir des dimensions de la fenêtre.
		auto windowSize = window_.getSize();
		float aspect = (float)windowSize.x / windowSize.y;

		// Appliquer la perspective avec un champs de vision (FOV) vertical donné et avec un aspect correspondant à celui de la fenêtre.
		projection.perspective(50, aspect, 0.01f, 100.0f);
		setMatrixOnAll("projection", projection);
	}

	void setMatrixOnAll(std::string_view name, const TransformStack& matrix) {
		// Les programmes de nuanceur ont leur propre espace mémoire. Il faut donc mettre à jour les variables uniformes pour chaque programme.
		for (auto&& prog : allPrograms) {
			prog.use();
			prog.setMat(name, matrix);
		}
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
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	// Pour des fins pédagogiques on désactive l'antialias automatique de OpenGL pour mieux illustrer le filtrage des textures.
	settings.context.antiAliasingLevel = 0;

	App app;
	app.run(argc, argv, "Exemple Semaine 4: Textures", settings);
}
