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


using namespace gl;
using namespace glm;


// Des couleurs
vec4 black = {0.0f, 0.0f, 0.0f, 1.0f};
vec4 darkGrey = {0.2f, 0.2f, 0.2f, 1.0f};
vec4 grey = {0.5f, 0.5f, 0.5f, 1.0f};
vec4 white = {1.0f, 1.0f, 1.0f, 1.0f};
vec4 brightRed = {1.0f, 0.2f, 0.2f, 1.0f};
vec4 brightGreen = {0.2f, 1.0f, 0.2f, 1.0f};
vec4 brightBlue = {0.2f, 0.2f, 1.0f, 1.0f};
vec4 brightYellow = {1.0f, 1.0f, 0.2f, 1.0f};


struct App : public OpenGLApplication
{
	// Les maillages des formes. Notez qu'on a deux maillages différent pour les cubes. En effet, si on ne veut pas voir les triangles des faces d'un cube en wireframe, il faut un autre maillage qui est dessiné avec des GL_LINES et qui dessine juste les arêtes du cube.
	Mesh pyramid;
	Mesh cubeBox;
	Mesh cubeWire;
	// On se fait un autre VBO pour les couleurs par sommet de la pyramide (les cubes ont des couleurs uniformes, pas par sommet).
	GLuint pyramidColorVbo = 0;

	// Les deux programmes : un pour les couleurs par sommet (pyramide), un pour les couleurs uniformes (cubes).
	ShaderProgram coloredVertexShaders;
	ShaderProgram solidColorShaders;

	// Les matrices de transformations.
	TransformStack model;
	TransformStack view;
	TransformStack projection;

	bool perspectiveCamera = true;
	float perspectiveVerticalFov = 50;
	float cameraDistance = 5;
	float cameraRoll = 0;
	float cameraLatitude = 0;
	float cameraLongitude = 0;

	float rotatingAngle = 0;

	// Appelée avant la première trame.
	void init() override {
		// Le message expliquant les touches de clavier.
		setKeybindMessage(
			"F5 : capture d'écran." "\n"
			"+ et - : rapprocher et éloigner la caméra orbitale." "\n"
			"+ et - avec Shift : réduire/élargir le champs de vision (FOV)." "\n"
			"haut/bas : changer l'élévation ou la latitude de la caméra orbitale." "\n"
			"gauche/droite : changer la longitude ou le roulement (avec shift) de la caméra orbitale." "\n"
			"R : Réinitialiser la position de la caméra." "\n"
			"1 : Projection perspective" "\n"
			"2 : Projection orthogonale" "\n"
		);

		// Config de base.
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		// Mettre des lignes assez visibles.
		glEnable(GL_POINT_SMOOTH);
		glPointSize(3.0f);
		glLineWidth(3.0f);
		// La couleur de fond.
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);

		loadShaders();
		initPyramid();
		initCube();

		applyPerspective();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// On veut une caméra qui orbite autour de l'origine et contrôlée par les touches de clavier.
		setupOrbitCamera();

		// Dessiner les trois cubes transformés.
		drawCubeScene();

		// Dessiner la pyramide qui tourne autour des z.
		drawRotatingPyramid();

		// Mettre à jour la rotation du cube rouge et de la pyramide.
		rotatingAngle += 2;
	}

	// Appelée lorsque la fenêtre se ferme.
	void onClose() override {
		pyramid.deleteObjects();
		cubeBox.deleteObjects();
		cubeWire.deleteObjects();
		glDeleteBuffers(1, &pyramidColorVbo);
		coloredVertexShaders.deleteShaders();
		coloredVertexShaders.deleteProgram();
		solidColorShaders.deleteShaders();
		solidColorShaders.deleteProgram();
	}

	// Appelée lors d'une touche de clavier.
	void onKeyPress(const sf::Event::KeyPressed& key) override {
		using enum sf::Keyboard::Key;
		switch (key.code) {
		// Les touches + et - servent à rapprocher et éloigner la caméra orbitale.
		// Les touches + et - avec Shift servent à réduire/élargir le champs de vision (FOV).
		case Add:
			if (key.shift) {
				perspectiveVerticalFov -= 5.0f;
				if (perspectiveCamera)
					applyPerspective();
				break;
			} else {
				cameraDistance -= 0.5f;
				if (not perspectiveCamera)
					// Avec une projection orthogonale, changer la distance de la caméra ne change pas le champs de vision, car celui-ci est contrôlé par la boîte de projection. Il faut donc changer la boîte de projection quand on change la distance de la caméra orbitale.
					applyOrtho();
			}
			break;
		case Subtract:
			if (key.shift) {
				perspectiveVerticalFov += 5.0f;
				if (perspectiveCamera)
					applyPerspective();
			} else {
				cameraDistance += 0.5f;
				if (not perspectiveCamera)
					applyOrtho();
			}
			break;
		// Les touches haut/bas change l'élévation ou la latitude de la caméra orbitale.
		case Up:
			cameraLatitude += 5.0f;
			break;
		case Down:
			cameraLatitude -= 5.0f;
			break;
		// Les touches gauche/droite change la longitude ou le roulement (avec shift) de la caméra orbitale.
		case Left:
			if (key.shift)
				cameraRoll -= 5.0f;
			else
				cameraLongitude += 5.0f;
			break;
		case Right:
			if (key.shift)
				cameraRoll += 5.0f;
			else
				cameraLongitude -= 5.0f;
			break;
		// Réinitialiser la position de la caméra.
		case R:
			cameraDistance = 5;
			cameraLatitude = 0;
			cameraLongitude = 0;
			cameraRoll = 0;
			perspectiveVerticalFov = 50;
			updateProjection();
			break;
		// 1 : Projection perspective
		case Num1:
			perspectiveCamera = true;
			updateProjection();
			std::cout << "Proj perspective" << "\n";
			break;
		// 2 : Projection orthogonale
		case Num2:
			perspectiveCamera = false;
			updateProjection();
			std::cout << "Proj ortho" << "\n";
			break;

		case F5: {
			std::string path = saveScreenshot();
			std::cout << "Capture d'écran dans " << path << std::endl;
			break;
		}}
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::Resized& event) override {
		// Mettre à jour la matrice de projection avec le nouvel aspect de fenêtre après le redimensionnement.
		updateProjection();
	}

	void initPyramid() {
		vec3 top =     { 0.0f,  0.3f,  0.0f};
		vec3 bottomR = {-0.3f, -0.1f, -0.1f};
		vec3 bottomL = { 0.3f, -0.1f, -0.1f};
		vec3 bottomF = { 0.0f, -0.1f,  0.7f};
		pyramid.vertices = {
			{top,     {}, {}},
			{bottomR, {}, {}},
			{bottomL, {}, {}},
			{bottomF, {}, {}},
		};
		pyramid.indices = {
			// Dessous
			1, 2, 3,
			// Face "tribord"
			1, 3, 0,
			// Face "babord"
			3, 2, 0,
			// Face arrière
			2, 1, 0,
		};
		pyramid.setup();

		// La classe Mesh configure les attributs de position, de normale et de coordonnées de texture pour chaque sommet. Ce sont les informations de base nécessaires pour afficher des formes. Dans notre cas, on veut de la couleur, donc un vec4 (RGBA) de plus pour chaque sommet. On peut ajouter un autre VBO qui passera les couleurs de sommets dans l'attribut de sommet à l'index 3.
		vec4 pyramidVertexColors[4] = {
			brightYellow,
			brightGreen,
			brightRed,
			brightBlue,
		};
		// Activer le contexte du VAO de la pyramide.
		pyramid.bindVao();
		// Créer et passer les données au VBO des couleurs.
		glGenBuffers(1, &pyramidColorVbo);
		glBindBuffer(GL_ARRAY_BUFFER, pyramidColorVbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVertexColors), pyramidVertexColors, GL_STATIC_DRAW);
		// Configurer l'attribut 3 avec des vec4.
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(3);
		pyramid.unbindVao();
	}

	void drawRotatingPyramid() {
		model.pushIdentity();
		// L'ordre des opérations est important. On veut faire un « barrel roll » (Starfox, do a barrel roll!). Il faut donc faire la rotation en premier puis faire la translation.
		model.rotate(rotatingAngle, {0, 0, 1});
		model.translate({0, -0.4f, 0});
		// Il faut faire la mise à l'échelle en dernier pour écraser la pyramide sur sa hauteur.
		model.scale({1, 0.8f, 1});
		setMatrix("model", model);
		model.pop();

		// Dessiner la pyramide avec le nuanceur avec couleur par sommet.
		coloredVertexShaders.use();
		pyramid.draw(GL_TRIANGLES);
		solidColorShaders.use();
		// Redessiner la pyramide avec le nuanceur avec couleur globale et en dessinant des lignes.
		solidColorShaders.setVec("globalColor", black);
		pyramid.draw(GL_LINE_STRIP);
	}

	void initCube() {
		vec3 backBottomR =  {-1, -1, -1};
		vec3 backTopR =     {-1,  1, -1};
		vec3 backBottomL =  { 1, -1, -1};
		vec3 backTopL =     { 1,  1, -1};
		vec3 frontBottomR = {-1, -1,  1};
		vec3 frontTopR =    {-1,  1,  1};
		vec3 frontBottomL = { 1, -1,  1};
		vec3 frontTopL =    { 1,  1,  1};

		cubeBox.vertices = {
			{backBottomR, {}, {}},
			{backTopR, {}, {}},
			{backBottomL, {}, {}},
			{backTopL, {}, {}},
			{frontBottomR, {}, {}},
			{frontTopR, {}, {}},
			{frontBottomL, {}, {}},
			{frontTopL, {}, {}},
		};
		cubeBox.indices = {
			// Faces arrière
			0, 1, 3,
			0, 3, 2,
			// Faces droite
			4, 5, 1,
			4, 1, 0,
			// Faces gauche
			2, 3, 7,
			2, 7, 6,
			// Faces avant
			6, 7, 5,
			6, 5, 4,
			// Faces dessus
			7, 3, 1,
			7, 1, 5,
			// Faces dessous
			6, 4, 0,
			6, 0, 2,
		};
		cubeBox.setup();

		// Configurer un deuxième mesh pour le wireframe du cube, de cette façon on ne dessine pas les lignes des triangles dans les faces et on dessine seulement les arêtes du cube.
		cubeWire.vertices = cubeBox.vertices;
		cubeWire.indices = {
			// Face arrière
			0, 1, 1, 3, 3, 2, 2, 0,
			// Face avant
			4, 5, 5, 7, 7, 6, 6, 4,
			// Arêtes de côté
			0, 4, 1, 5, 3, 7, 2, 6
		};
		cubeWire.setup();
	}

	void drawCubeScene() {
		model.pushIdentity();

		// Faire une translation en premier pour déplacer toute la scène. Bon exemple de la raison d'utiliser une pile de matrices. 
		model.translate({0, -1, 0});

		// Dupliquer la matrice courante qui contient la translation globale.
		model.push();
		// Écraser le cube en Y.
		model.scale({1.2f, 0.1f, 1.2f});
		// Dessiner le cube gris.
		drawCube(grey);
		model.pop();

		model.push();
		// Appliquer la translation pour mettre le cube rouge dans le coin. Il faut faire cette translation en premier.
		model.translate({-0.7f, 0.5f, -0.7f});
		// Appliquer la rotation après la translation. De cette façon, le prisme rouge tourne sur lui-même en Y.
		model.rotate(rotatingAngle, {0, 1, 0});
		// Appliquer la mise à l'échelle en dernier pour avoir une forme rectangulaire.
		model.scale({0.3f, 0.5f, 0.2f});
		// Dessiner le cube rouge.
		drawCube(brightRed);
		model.pop();

		model.push();
		model.translate({0.5f, 0.2f, 0.5f});
		model.scale({0.4f, 0.2f, 0.4f});
		drawCube(brightGreen);
		model.pop();

		model.pop();
	}

	void drawCube(vec4 cubeColor) {
		auto& prog = solidColorShaders;
		// Choisir le nuanceur à couleur globale.
		solidColorShaders.use();
		// Mettre à jour les variables uniformes de matrice de modélisation et de couleur.
		solidColorShaders.setMat("model", model);
		solidColorShaders.setVec("globalColor", cubeColor);
		// Dessiner le cube solide.
		cubeBox.draw(GL_TRIANGLES);
		// Changer la couleur globale au noir pour dessiner les arêtes.
		solidColorShaders.setVec("globalColor", black);
		// Dessiner les arêtes (le wireframe) avec le mesh qui correspond aux lignes.
		cubeWire.draw(GL_LINES);
	}

	void setupOrbitCamera() {
		view.pushIdentity();
		// Comme pour le reste, l'ordre des opérations est important.
		view.translate({0, 0, -cameraDistance});
		view.rotate(cameraRoll,      {0, 0, 1});
		view.rotate(cameraLatitude,  {1, 0, 0});
		view.rotate(cameraLongitude, {0, 1, 0});
		// En positionnant la caméra, on met seulement à jour la matrice de visualisation.
		setMatrix("view", view);
		view.pop();
	}

	void applyPerspective() {
		// Calculer l'aspect de notre caméra à partir des dimensions de la fenêtre.
		auto windowSize = window_.getSize();
		float aspect = (float)windowSize.x / windowSize.y;

		projection.pushIdentity();
		// Appliquer la perspective avec un champs de vision (FOV) vertical donné et avec un aspect correspondant à celui de la fenêtre.
		projection.perspective(perspectiveVerticalFov, aspect, 0.1f, 100.0f);
		setMatrix("projection", projection);
		projection.pop();
	}

	void applyOrtho() {
		// Calculer l'aspect de notre caméra à partir des dimensions de la fenêtre.
		auto windowSize = window_.getSize();
		float aspect = (float)windowSize.x / windowSize.y;

		projection.pushIdentity();
		// Construire une boîte de projection en utilisant la distance de la caméra pour que les touches + et - fassent au moins une sorte de zoom.
		float boxEdge = cameraDistance / 3;
		ProjectionBox box = {-boxEdge, boxEdge, -boxEdge, boxEdge, 0.1f, 100.0f};
		// Appliquer l'aspect aux limites horizontales.
		box.leftFace *= aspect;
		box.rightFace *= aspect;
		projection.ortho(box);
		setMatrix("projection", projection);
		projection.pop();
	}

	void updateProjection() {
		if (perspectiveCamera)
			applyPerspective();
		else
			applyOrtho();
	}

	void setMatrix(std::string_view name, const TransformStack& matrix) {
		// Les programmes de nuanceur ont leur propre espace mémoire. Il faut donc mettre à jour les variables uniformes pour chaque programme.
		coloredVertexShaders.use();
		coloredVertexShaders.setMat(name, matrix);
		solidColorShaders.use();
		solidColorShaders.setMat(name, matrix);
	}

	void loadShaders() {
		// Créer le programme nuanceur pour les couleurs par sommet.
		coloredVertexShaders.create();
		// Compiler et attacher le nuanceur de sommets. Il sera réutilisé
		auto basicVertexShader = coloredVertexShaders.attachSourceFile(GL_VERTEX_SHADER, "basic_vert.glsl");
		// Compiler et attacher le nuanceur de fragments qui utilise une couleur d'entrée.
		coloredVertexShaders.attachSourceFile(GL_FRAGMENT_SHADER, "vertex_color_frag.glsl");
		coloredVertexShaders.link();

		// Créer le programme nuanceur pour les couleurs solides.
		solidColorShaders.create();
		// Attacher le nuanceur de sommets précédent pour ne pas avoir à le recompiler.
		solidColorShaders.attachExistingShader(GL_VERTEX_SHADER, basicVertexShader);
		// Compiler et attacher le nuanceur de fragments qui utilise une couleur uniforme.
		solidColorShaders.attachSourceFile(GL_FRAGMENT_SHADER, "solid_color_frag.glsl");
		solidColorShaders.link();
	}
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	settings.context.antiAliasingLevel = 4;

	App app;
	app.run(argc, argv, "Exemple Semaine 3: Transformations", settings);
}
