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


// Un matériau d'objet dans la scène.
struct Material
{
	// Couleur émise par l'objet même en l'absence de source lumineuse.
	vec4 emissionColor;
	// Couleur reflétée peut importe la distance ou direction des sources lumineuses.
	vec4 ambientColor;
	// Couleur reflétée par une lumière reçue d'une direction et réfléchie également dans toutes les directions.
	vec4 diffuseColor;
	// Couleur reflétée par une lumière reçue d'une direction et réfléchie dans une direction.
	vec4 specularColor;
	// Exposant de réflexion spéculaire
	float shininess;
};

// Une source lumineuse dans la scène. Typiquement, les couleurs ambiante, diffuse et spéculaire d'une source lumineuse sont les mêmes. Ce sont plutôt les matériaux qui réfléchissent différentes couleurs. On se laisse quand même l'option de paramétrer ça dans les sources lumineuses.
struct LightSource
{
	// La position en coordonnées de scène.
	vec4 position;
	// Couleur ambiante émise par la source.
	vec4 ambientColor;
	// Couleur diffuse émise par la source.
	vec4 diffuseColor;
	// Couleur spéculaire émise par la source.
	vec4 specularColor;
	// Coefficents d'atténuation constante, linéaire et quadratique selon la distance.
	float fadeCst;
	float fadeLin;
	float fadeQuad;
	// Direction (seulement utile pour les "spots").
	vec4 direction;
	// Angle d'ouverture (seulement utile pour les "spots").
	float beamAngle;
	// Exposant du cône de lumière (seulement utile pour les "spots").
	float exponent;
};

// Les paramètres du modèle d'éclairage. C'est ici qu'on peut mettre des options sur comment l'éclairage est fait.
struct LightModel
{
	// La couleur ambiante de la scène, indépendamment des sources lumineuses présentes.
	vec4 ambientColor;
	// Le modèle de calcul du vecteur de l'observateur. Allez voir les détails dans les nuanceurs.
	// C'est un bool dans les nuanceurs et un int ici. Le modèle de données d'OpenGL (std140) aligne les booléens sur 4 octects.
	uint32_t localViewer;
};

struct App : public OpenGLApplication
{
	Mesh shapeFlat;
	Mesh normalsFlat;
	Mesh shapeSmooth;
	Mesh normalsSmooth;
	Mesh lightBulb;

	UniformBlock<Material> material = {"Material", 0};
	UniformBlock<LightSource> light = {"LightSource", 1};
	UniformBlock<LightModel> lightModel = {"LightModel", 2};

	ShaderProgram uniformProg;
	ShaderProgram flatProg;
	ShaderProgram gouraudProg;
	ShaderProgram phongProg;
	ShaderProgram* programs[4] = {&uniformProg, &flatProg, &gouraudProg, &phongProg};
	ShaderProgram* currentProg = &flatProg;

	TransformStack model = {"model"};
	TransformStack view = {"view"};
	TransformStack projection = {"projection"};

	OrbitCamera camera = {10, 30, 30, 0};

	int drawMode = 2;
	Uniform<bool> showingAmbientReflection = {"showingAmbientReflection", true};
	Uniform<bool> showingDiffuseReflection = {"showingDiffuseReflection", true};
	Uniform<bool> showingSpecularReflection = {"showingSpecularReflection", true};
	Uniform<bool> usingSmoothNormals = {"usingSmoothNormals", false};
	Uniform<bool> usingBlinnFormula = {"usingBlinnFormula", true};
	Uniform<int> numCelShadingBands = {"numCelShadingBands", 1};
	Uniform<mat3> normalTransformMat = {"normalTransformMat"};
	Uniform<vec4> globalColor = {"globalColor"};

	// Appelée avant la première trame.
	void init() override {
		setKeybindMessage(
			"R : réinitialiser la position de la caméra." "\n"
			"+ et - :  rapprocher et éloigner la caméra orbitale." "\n"
			"haut/bas : changer la latitude de la caméra orbitale." "\n"
			"gauche/droite : changer la longitude ou le roulement (avec shift) de la caméra orbitale." "\n"
			"clic central (cliquer la roulette) : bouger la caméra en glissant la souris." "\n"
			"roulette : rapprocher et éloigner la caméra orbitale." "\n"
			"1 : Démo des modèles de calcul de normales" "\n"
			"2 : Modèle plat ou flat shading (calculs par primitive)" "\n"
			"3 : Modèle de Gouraud (calculs par sommet)" "\n"
			"4 : Modèle de Phong (calculs par fragment)" "\n"
			"W et S : Bouger la lumière sur l'axe des Z" "\n"
			"A et D : Tourner la sphère autour de l'axe des Y" "\n"
			"N : Changer le types de normales" "\n"
			"      - 'smooth' : interpolées aux sommets" "\n"
			"      - 'flat' : perpendiculaire à la face" "\n"
			"B : Changer la formule de calcul spéculaire entre Blinn et Phong" "\n"
			"J : Activer/désactiver la réflexion ambiante" "\n"
			"K : Activer/désactiver la réflexion diffuse" "\n"
			"L : Activer/désactiver la réflexion spéculaire" "\n"
			"U : Augmenter le nombre de bande de cel-shading" "\n"
			"I : Diminuer le nombre de bande de cel-shading (0 = pas de cel-shading)" "\n"
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

		// On a deux sphères dont les géométries sont identiques, mais dont les normales sont calculées différemment. "smooth" veut dire normales interpolées aux sommets et "flat" veut dire normales perpendiculaires à la surface des primitives. Cette information est précaculée (par Blender par exemple) et encodée dans le mesh lui-même (VertexData::normal).
		shapeFlat = Mesh::loadFromWavefrontFile("sphere_flat.obj")[0];
		shapeSmooth = Mesh::loadFromWavefrontFile("sphere_smooth.obj")[0];
		buildNormalLines(0.5f);

		// Initialiser le matériau.
		material = Material{
			{0.0, 0.1, 0.0, 1.0},
			{0.2, 0.1, 0.1, 1.0},
			{0.3, 0.7, 0.3, 1.0},
			{0.5, 0.5, 1.0, 1.0},
			50
		};
		material.setup();

		// Initialiser la lumière.
		light = LightSource{
			{0.0, 0.0, 5.0, 1.0},
			{1.0, 1.0, 1.0, 1.0},
			{1.0, 1.0, 1.0, 1.0},
			{1.0, 1.0, 1.0, 1.0},
			0.001,
			0.005,
			0.01,
			{0, 0, -1, 1},
			20,
			1,
		};
		light.setup();

		// Initialiser le modèle d'éclairage. dans ce cas on n'utilise pas de couleur ambiante (c'est la source lumineuse qui l'a).
		lightModel = LightModel{{0, 0, 0, 1}, true};
		lightModel.setup();

		// Lier chacun des blocs uniformes aux variables uniformes des nuanceurs.
		for (auto&& prog : programs) {
			material.bindToProgram(*prog);
			light.bindToProgram(*prog);
			lightModel.bindToProgram(*prog);
		}

		// Caméra et projection habituelles.
		for (auto&& prog : programs)
			camera.updateProgram(*prog, view);
		applyPerspective();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		switch (drawMode) {
		case 1:
			drawUnlitShapeWithNormalLines();
			break;
		case 2:
		case 3:
		case 4:
			currentProg->use();
			// Passer les variables uniformes de contrôle.
			currentProg->setUniform(showingAmbientReflection);
			currentProg->setUniform(showingDiffuseReflection);
			currentProg->setUniform(showingSpecularReflection);
			currentProg->setUniform(usingBlinnFormula);
			currentProg->setUniform(numCelShadingBands);
			// Passer la transposée de l'inverse de la matrice modèle-vue pour transformer les normales avec l'objet.
			normalTransformMat = transpose(inverse(mat3(view * model)));
			currentProg->setUniform(normalTransformMat);
			currentProg->setMat(model);
			// Dessiner la sphère éclairée avec les normales "flat" ou "smooth", selon le mode choisi.
			if (usingSmoothNormals)
				shapeSmooth.draw();
			else
				shapeFlat.draw();

			// Dessiner la lumière avec une couleur uniforme (la lumière ne s'éclaire pas elle-même, pour simplifier l'affichage).
			uniformProg.use();
			model.push(scale(translate(mat4(1), vec3(light->position)), {0.2, 0.2, 0.2}));
			uniformProg.setMat(model);
			model.pop();
			globalColor = {1, 1, 0.7, 1};
			uniformProg.setUniform(globalColor);
			shapeFlat.draw();

			break;
		}
	}

	// Appelée lors d'une touche de clavier.
	void onKeyPress(const sf::Event::KeyEvent& key) override {
		// La touche R réinitialise la position de la caméra.
		// Les touches + et - rapprochent et éloignent la caméra orbitale.
		// Les touches haut/bas change l'élévation ou la latitude de la caméra orbitale.
		// Les touches gauche/droite change la longitude ou le roulement (avec shift) de la caméra orbitale.
		// 
		// W et S : Bouger la lumière sur l'axe des Z
		// A et D : Tourner la sphère autour de l'axe des Y
		// 1 : Démo des modèles de calcul de normales
		// 2 : Modèle plat ou flat shading (calculs par primitive)
		// 3 : Modèle de Gouraud (calculs par sommet)
		// 4 : Modèle de Phong (calculs par fragment)
		// N : Changer le types de normales
		//       - "smooth" : interpolées aux sommets
		//       - "flat" : perpendiculaire à la face)
		// B : Changer la formule de calcul spéculaire entre Blinn et Phong
		// J : Activer/désactiver la réflexion ambiante
		// K : Activer/désactiver la réflexion diffuse
		// L : Activer/désactiver la réflexion spéculaire
		// U : Augmenter le nombre de bande de cel-shading
		// I : Diminuer le nombre de bande de cel-shading (0 = pas de cel-shading)

		camera.handleKeyEvent(key, 5, 0.5f, {10, 90, 180, 0});
		
		using enum sf::Keyboard::Key;
		switch (key.code) {
		case A:
			model.rotate(5, {0, 1, 0});
			break;
		case D:
			model.rotate(-5, {0, 1, 0});
			break;
		case W:
			light->position.z += 0.5;
			light.updateBuffer();
			break;
		case S:
			light->position.z -= 0.5;
			light.updateBuffer();
			break;
		case R:
			model.loadIdentity();
			light->position.z = 5;
			light.updateBuffer();
			break;

		case Num1:
			drawMode = 1;
			currentProg = &uniformProg;
			std::cout << "Démo des normales" << "\n";
			break;
		case Num2:
			// Dans le modèle plat, les calculs sont effectués une fois par primitive, donc dans le nuanceur de géométrie. Les résultats sont passés également à tous les fragments de la primitive.
			drawMode = 2;
			currentProg = &flatProg;
			std::cout << "Modèle plat (flat shading)" << "\n";
			break;
		case Num3:
			// Dans le modèle de Gouraud, les calculs sont effectués une fois par sommet, donc dans le nuanceur de sommets. Les résultats (couleurs) sont passés en sortie des sommets et donc interpolés aux fragments.
			drawMode = 3;
			currentProg = &gouraudProg;
			std::cout << "Modèle de Gouraud" << "\n";
			break;
		case Num4:
			// Dans le modèle de Phong, les calculs géométriques (positions, directions, normales) sont faits dans le nuanceur de sommet et interpolés aux fragments. C'est ensuite dans chaque fragment que sont effectués les calculs d'éclairage avec les vecteurs interpolés en entrée.
			drawMode = 4;
			currentProg = &phongProg;
			std::cout << "Modèle de Phong" << "\n";
			break;

		case N:
			usingSmoothNormals ^= 1;
			std::cout << (usingSmoothNormals ? "Normales lisses" : "Normales plates") << "\n";
			break;
		case B:
			usingBlinnFormula ^= 1;
			std::cout << (usingBlinnFormula ? "Formule de Blinn" : "Formule de Phong") << "\n";
			break;

		case J:
			showingAmbientReflection ^= 1;
			std::cout << "Réflexion ambiante " << (showingAmbientReflection ? "ON" : "OFF") << "\n";
			break;
		case K:
			showingDiffuseReflection ^= 1;
			std::cout << "Réflexion diffuse " << (showingDiffuseReflection ? "ON" : "OFF") << "\n";
			break;
		case L:
			showingSpecularReflection ^= 1;
			std::cout << "Réflexion spéculaire " << (showingSpecularReflection ? "ON" : "OFF") << "\n";
			break;

		case U:
			numCelShadingBands = std::max(1, numCelShadingBands + 1);
			std::cout << "Cel-shading : " << numCelShadingBands << "\n";
			break;
		case I:
			numCelShadingBands = std::max(1, numCelShadingBands - 1);
			std::cout << "Cel-shading : " << numCelShadingBands << "\n";
			break;
		}

		for (auto&& prog : programs)
			camera.updateProgram(*prog, view);
	}

	// Appelée lors d'un mouvement de souris.
	void onMouseMove(const sf::Event::MouseMoveEvent& mouseDelta) override {
		// Mettre à jour la caméra si on a un clic de la roulette.
		auto& mouse = getMouse();
		camera.handleMouseMoveEvent(mouseDelta, mouse, deltaTime_ / (0.7f / 30));
		for (auto&& prog : programs)
			camera.updateProgram(*prog, view);
	}

	// Appelée lors d'un défilement de souris.
	void onMouseScroll(const sf::Event::MouseWheelScrollEvent& mouseScroll) override {
		// Zoom in/out
		camera.altitude -= mouseScroll.delta;
		for (auto&& prog : programs)
			camera.updateProgram(*prog, view);
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::SizeEvent& event) override {
		applyPerspective();
	}

	void loadShaders() {
		uniformProg.create();
		uniformProg.attachSourceFile(GL_VERTEX_SHADER, "basic_vert.glsl");
		uniformProg.attachSourceFile(GL_FRAGMENT_SHADER, "uniform_frag.glsl");
		uniformProg.link();

		// Le nuanceur de fragments pour le flat shading prend en entrée la couleur (venant du nuanceur de géométrie) et l'affecte telle-quelle en sortie. On réutilise donc le nuanceur de fragments de Gouraud.
		flatProg.create();
		flatProg.attachSourceFile(GL_VERTEX_SHADER, "flat_vert.glsl");
		flatProg.attachSourceFile(GL_GEOMETRY_SHADER, "flat_geom.glsl");
		flatProg.attachSourceFile(GL_FRAGMENT_SHADER, "gouraud_frag.glsl");
		flatProg.link();

		gouraudProg.create();
		gouraudProg.attachSourceFile(GL_VERTEX_SHADER, "gouraud_vert.glsl");
		gouraudProg.attachSourceFile(GL_FRAGMENT_SHADER, "gouraud_frag.glsl");
		gouraudProg.link();

		phongProg.create();
		phongProg.attachSourceFile(GL_VERTEX_SHADER, "phong_vert.glsl");
		phongProg.attachSourceFile(GL_FRAGMENT_SHADER, "phong_frag.glsl");
		phongProg.link();
	}

	void drawUnlitShapeWithNormalLines() {
		currentProg->use();
		model.pushIdentity();
		currentProg->setMat(model);
		model.pop();

		// Dessiner la sphère avec une couleur uniforme (rouge).
		globalColor = {1, 0.2, 0.2, 1};
		currentProg->setUniform(globalColor);
		shapeSmooth.draw(GL_TRIANGLES);

		// Dessiner les arêtes des primitives en noir en dessinant la sphère en wireframe.
		globalColor = {0, 0, 0, 1};
		currentProg->setUniform(globalColor);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		shapeSmooth.draw(GL_TRIANGLES);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// Dessiner les droites de normales selon le type voulu.
		globalColor = {0.2, 1, 0.2, 1};
		currentProg->setUniform(globalColor);
		if (*usingSmoothNormals)
			normalsSmooth.draw(GL_LINES);
		else
			normalsFlat.draw(GL_LINES);
	}

	void applyPerspective(float fovy = 50) {
		// Appliquer la perspective avec un champs de vision (FOV) vertical donné et avec un aspect correspondant à celui de la fenêtre.
		projection.perspective(fovy, getWindowAspect(), 0.01f, 100.0f);
		for (auto&& prog : programs) {
			prog->use();
			prog->setMat(projection);
		}
	}

	void buildNormalLines(float lineLength = 1) {
		// Construire les droites pour la visualisation des normales par sommet ("smooth")
		for (auto& v : shapeSmooth.vertices) {
			normalsSmooth.vertices.push_back({v.position, {}, {}});
			normalsSmooth.vertices.push_back({v.position + v.normal * lineLength, {}, {}});
		}

		// Construire les droites pour la visualisation des normales par face ("face"). Pour illustrer, on place les droites aux centroïdes des primitives.
		for (size_t i = 0; i < shapeFlat.vertices.size(); i += 3) {
			auto& v0 = shapeFlat.vertices[i + 0];
			auto& v1 = shapeFlat.vertices[i + 1];
			auto& v2 = shapeFlat.vertices[i + 2];
			vec3 faceCentroid = (v0.position + v1.position + v2.position) / 3.0f;
			normalsFlat.vertices.push_back({faceCentroid, {}, {}});
			normalsFlat.vertices.push_back({faceCentroid + v0.normal * lineLength, {}, {}});
		}

		normalsSmooth.setup();
		normalsFlat.setup();
	}
};


void runDiffuseModelTest(const std::string& filename, vec3 posL, vec3 n1, vec3 n2, vec3 p1, vec3 p2) {
	n1 = normalize(n1);
	n2 = normalize(n2);

	std::vector<float> xs;
	for (float x = p1.x; x <= p2.x; x += 0.1f)
		xs.push_back(x);

	// Avec Gouraud, on calcule les couleurs aux sommets et on interpole dans les fragments.
	std::vector<float> gouraud;
	float c1 = dot(n1, normalize(posL - p1));
	float c2 = dot(n2, normalize(posL - p2));
	for (float x : xs) {
		float xValue = (x - p1.x) / (p2.x - p1.x);
		float diffuse = mix(c1, c2, xValue);
		gouraud.push_back(std::clamp(diffuse, 0.0f, 1.0f));
	}

	// Avec Phong, on calcule les vecteurs (L pour le diffus) aux sommets et on calcule les couleurs dans les fragments avec les vecteurs interpolés.
	std::vector<float> phong;
	vec3 l1 = posL - p1;
	vec3 l2 = posL - p2;
	for (float x : xs) {
		float xValue = (x - p1.x) / (p2.x - p1.x);
		vec3 l = normalize(mix(l1, l2, xValue));
		vec3 n = normalize(mix(n1, n2, xValue));
		float diffuse = dot(n, l);
		phong.push_back(std::clamp(diffuse, 0.0f, 1.0f));
	}

	std::ofstream file(filename);
	file << "x\tGouraud\tPhong" << "\n";
	for (size_t i = 0; i < xs.size(); i++)
		file << std::format("{}\t{}\t{}\n", xs[i], gouraud[i], phong[i]);
}


int main(int argc, char* argv[]) {
	// Vous pouvez utiliser cette fonction pour générer des courbes d'interpolations selon les modèles de Gouraud et Phong. Les données sont ensuite importables dans Excel pour la visualisation.
	runDiffuseModelTest(
		"test1.csv",
		{0, 4, 0},
		{0, 1, 0},
		{1, 1, 0},
		{-4, 0, 0},
		{ 4, 0, 0}
	);
	runDiffuseModelTest(
		"test2.csv",
		{0, 4, 0},
		{0, 1, 0},
		{1, 2, 0},
		{-4, 0, 0},
		{4, 0, 0}
	);

	WindowSettings settings = {};
	settings.fps = 30;
	settings.context.antialiasingLevel = 4;

	App app;
	app.run(argc, argv, "Exemple Semaine 7: Illumination", settings);
}
