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
#include <inf2705/OrbitCamera.hpp>


using namespace gl;
using namespace glm;


struct App : public OpenGLApplication
{
	static constexpr int gridSize = 30;

	Mesh gridLines;
	Mesh gridPoints;
	Mesh referenceLines;
	bool activeFrags[gridSize][gridSize] = {};
	GLuint activeFragsVbo = 0;

	ShaderProgram globalColorProg;
	ShaderProgram quadGenProg;

	TransformStack model = {"model"};
	TransformStack view = {"view"};
	TransformStack projection = {"projection"};

	Uniform<vec4> globalColor = {"globalColor", vec4(1, 1, 1, 1)};
	int currentLine = 0;
	bool usingBresenham = true;
	bool drawingCircles = false;

	// Appelée avant la première trame.
	void init() override {
		setKeybindMessage(
			"F5 : capture d'écran." "\n"
			"1 : droite où m = 0" "\n"
			"2 : droite où 0 < m < 1" "\n"
			"3 : droite où m = 1" "\n"
			"4 : droite où m > 1" "\n"
			"5 : droite où -1 < m < 0" "\n"
			"6 : droite où m = -1" "\n"
			"7 : droite où m < -1" "\n"
			"8 : droite où x1 > x2, 0 < m < 1" "\n"
			"9 : droite où x1 > x2, m < -1" "\n"
			"C : changer entre les exemples de cercles et de droites." "\n"
			"B : utiliser l'algo de Bresenham (avec entiers et symétries) ou incrémental (avec des réels)" "\n"
		);

		// Pas de test de profondeur pour aujourd'hui, on fait tout en 2D.
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glClearColor(0.75, 0.75, 0.75, 1);

		loadShaders();

		referenceLines.setup();

		for (int i = 0; i <= gridSize; i++) {
			gridLines.vertices.push_back({{i, 0, 0}});
			gridLines.vertices.push_back({{i, gridSize, 0}});
			gridLines.vertices.push_back({{0, i, 0}});
			gridLines.vertices.push_back({{gridSize, i, 0}});
		}
		gridLines.setup();

		for (int x = 0; x < gridSize; x++)
			for (int y = 0; y < gridSize; y++)
				gridPoints.vertices.push_back({{x + 0.5f, y + 0.5f, 0}});
		gridPoints.setup();

		gridPoints.bindVao();
		glGenBuffers(1, &activeFragsVbo);
		glBindBuffer(GL_ARRAY_BUFFER, activeFragsVbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(activeFrags), activeFrags, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(3, 1, GL_UNSIGNED_BYTE, GL_FALSE, 1, 0);
		glEnableVertexAttribArray(3);

		applyOrtho();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// Réinitialiser la grille de fragments et les lignes de référence.
		std::fill_n((uint8_t*)activeFrags, sizeof(activeFrags), 0);
		referenceLines.vertices.clear();

		// Points de droites.
		static std::pair<ivec2, ivec2> lines[] = {
			{{ 5,  5}, {15,  5}}, // m = 0
			{{ 5,  5}, {15, 10}}, // 0 < m < 1
			{{ 5,  5}, {15, 15}}, // m = 1
			{{ 5,  5}, {15, 25}}, // m > 1
			{{ 5, 25}, {15, 20}}, // -1 < m < 0
			{{ 5, 25}, {15, 15}}, // m = -1
			{{ 5, 25}, {15,  5}}, // m < -1
			{{15, 10}, { 5,  5}}, // x1 > x2, 0 < m < 1
			{{15,  5}, { 5, 25}}, // x1 > x2, m < -1
		};
		// Rayon et centre de cercles.
		static std::pair<int, ivec2> circles[] = {
			{5,  { 0,  0}},
			{10, { 0,  0}},
			{0,  {15, 15}},
			{1,  {15, 15}},
			{10, {15, 15}},
			{10, {10, 15}},
			{10, {15, 10}},
			{15, {10, 15}},
			{15, {15, 10}},
		};

		if (drawingCircles) {
			// Créer les fragment d'un cercle selon l'algo du point milieu.
			int r = circles[currentLine].first;
			ivec2 c = circles[currentLine].second;
			fillCircleFragments(r, c);
			// Ajouter un cercle bleu lisse pour illustrer la pixelisation.
			addReferenceCircle(r, c);
		} else {
			ivec2 p1 = lines[currentLine].first;
			ivec2 p2 = lines[currentLine].second;
			// Calculer les fragments d'une droite selon l'algorithme d'incrément initial ou Bresenham.
			if (usingBresenham)
				fillLineFragmentsBresenham(p1.x, p1.y, p2.x, p2.y);
			else
				fillLineFragmentsIncremental(p1.x, p1.y, p2.x, p2.y);
			// Ajouter une ligne de référence du milieu du pixel 1 au pixel 2 pour illustrer.
			addReferenceLine(p1.x, p1.y, p2.x, p2.y);
		}

		// Dessiner les quads qui représente les "fragments".
		drawFragments();
		// Dessiner une grille par-dessus les fragments pour les différencier.
		drawGrid();
		// Dessiner les lignes de référence par-dessus les autres éléments.
		drawReferenceLines();
	}

	// Appelée lorsque la fenêtre se ferme.
	void onClose() override {
		gridLines.deleteObjects();
		gridPoints.deleteObjects();
		glDeleteBuffers(1, &activeFragsVbo);
		activeFragsVbo = 0;
		referenceLines.deleteObjects();
		globalColorProg.deleteShaders();
		globalColorProg.deleteProgram();
	}

	// Appelée lors d'une touche de clavier.
	void onKeyPress(const sf::Event::KeyPressed& key) override {
		using enum sf::Keyboard::Key;
		switch (key.code) {
		case Num1:
		case Num2:
		case Num3:
		case Num4:
		case Num5:
		case Num6:
		case Num7:
		case Num8:
		case Num9:
			currentLine = (int)key.code - (int)Num1;
			std::cout << "Ligne " << (currentLine + 1) << "\n";
			break;
		case C:
			drawingCircles ^= 1;
			break;
		case B:
			usingBresenham ^= 1;
			std::cout << "Bresenham : " << std::boolalpha << usingBresenham << "\n";
			break;

		case F5:
			std::string path = saveScreenshot();
			std::cout << "Capture d'écran dans " << path << std::endl;
			break;
		}
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::Resized& event) override {
		applyOrtho();
	}

	void fillLineFragmentsIncremental(int x1, int y1, int x2, int y2) {
		auto fillFragment = [&](float x, float y) {
			// S'assurer que le fragment à allumer est bien dans la grille.
			if (0 <= x and x < gridSize and 0 <= y and y < gridSize)
				activeFrags[lround(x)][lround(y)] = true;
		};

		float m = ((float)y2 - y1) / (x2 - x1);
		float x = (float)x1;
		float y = (float)y1;
		fillFragment(x, y);
		while (x < x2) {
			x += 1;
			y += m;
			fillFragment(x, y);
		}
	}

	void fillLineFragmentsBresenham(int x1, int y1, int x2, int y2) {
		bool swapXY = false;
		bool decrementY = false;

		auto fillFragment = [&](int x, int y) {
			// Pour les pente > 1, on balaye sur y plutôt que x, donc il faut aussi inverser l'affectation pour ne pas avoir à changer l'algorithme.
			if (swapXY)
				std::swap(x, y);
			// S'assurer que le fragment à allumer est bien dans la grille.
			if (0 <= x and x < gridSize and 0 <= y and y < gridSize)
				activeFrags[x][y] = true;
		};

		// Si la pente est > 1, balayer par rapport à y plutôt que x, donc faire à semblant que les x sont des y et vice-versa.
		if (abs(y2 - y1) > abs(x2 - x1)) {
			swapXY = true;
			std::swap(x1, y1);
			std::swap(x2, y2);
		}
		// Si le deuxième point (x2,y2) est à gauche, inverser l'ordre des points.
		if (x2 < x1) {
			std::swap(x1, x2);
			std::swap(y1, y2);
		}

		int dx = x2 - x1;
		int dy = y2 - y1;
		int yincr = 1;
		// Si la pente est négative, décrémenter plutôt qu'incrémenter (donc sud-est plutôt que nord-est).
		if (dy < 0) {
			yincr = -1;
			dy = -dy;
		}
		int d = 2 * dy - dx;
		fillFragment(x1, y1);
		while (x1 < x2) {
			if (d <= 0) { // EST
				d = d + 2 * dy;
			} else { // NORD-EST ou SUD-EST
				d = d + 2 * (dy - dx);
				y1 = y1 + yincr;
			}
			x1 = x1 + 1;
			fillFragment(x1, y1);
		}
	}

	void fillCircleFragments(int radius, ivec2 center) {
		auto fillFragment = [&](int x, int y) {
			// Appliquer un décalage uniformément selon le centre du cercle.
			x += center.x;
			y += center.y;
			// S'assurer que le fragment à allumer est bien dans la grille.
			if (0 <= x and x < gridSize and 0 <= y and y < gridSize)
				activeFrags[x][y] = true;
		};

		// L'algorithme de base qui est implémenté ci-dessous allume seulement les fragments pour l'octant Nord-Nord-Est. Cependant, un cercle est nécessairement symétrique sur tous ces octants, donc pour chaque fragment, on allume aussi les 8 symétries.
		auto fillFragmentAndSymmetries = [&](int x, int y) {
			fillFragment( x,  y);
			fillFragment( y,  x);
			fillFragment(-x,  y);
			fillFragment(-y,  x);
			fillFragment( x, -y);
			fillFragment( y, -x);
			fillFragment(-x, -y);
			fillFragment(-y, -x);
		};

		int x = 0;
		int y = radius;
		int h = 1 - radius;
		fillFragmentAndSymmetries(x, y);
		while (y > x) { // ON RESTE DANS 2e OCTANT
			if (h < 0) { // EST
				h = h + 2 * x + 3;
			} else { // SUD-EST
				h = h + 2 * (x - y) + 5;
				y = y - 1;
			}
			x = x + 1;
			fillFragmentAndSymmetries(x, y);
		}
	}

	void addReferenceLine(int x1, int y1, int x2, int y2) {
		// Ajouter les droites de références avec leurs extrémités au milieu des fragments (d'où le +0.5).
		referenceLines.vertices.push_back({{x1 + 0.5f, y1 + 0.5f, 0}});
		referenceLines.vertices.push_back({{x2 + 0.5f, y2 + 0.5f, 0}});
	}

	void addReferenceCircle(int radius, ivec2 center) {
		// Ajouter les sommets d'un cercle avec une certaine résolution (nombre de sections).
		static const int numSections = 256;
		static const float sectionAngle = 360.0f / numSections;

		vec3 offset = vec3(center.x + 0.5, center.y + 0.5, 0);
		TransformStack rotation;
		rotation.rotate(sectionAngle, {0, 0, 1});

		vec4 p0 = vec4(radius, 0, 0, 1);
		vec4 p1 = p0;
		for (int i = 0; i < numSections; i++) {
			p1 = rotation * p0;
			referenceLines.vertices.push_back({{p0.xyz() + offset}});
			referenceLines.vertices.push_back({{p1.xyz() + offset}});
			p0 = p1;
		}
	}

	void drawFragments() {
		quadGenProg.use();
		gridPoints.bindVao();
		glBindBuffer(GL_ARRAY_BUFFER, activeFragsVbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(activeFrags), activeFrags, GL_DYNAMIC_DRAW);
		gridPoints.draw(GL_POINTS);
	}

	void drawReferenceLines() {
		static const vec4 lineColor = {0.0, 0.0, 0.75, 1};

		glEnable(GL_LINE_SMOOTH);
		glLineWidth(5);

		referenceLines.updateBuffers();
		globalColorProg.use();
		globalColor = lineColor;
		globalColorProg.setUniform(globalColor);
		model.loadIdentity();
		globalColorProg.setUniform(model);
		referenceLines.draw(GL_LINES);
	}

	void drawGrid() {
		glDisable(GL_LINE_SMOOTH);
		glEnable(GL_POINT_SMOOTH);
		glLineWidth(2);
		glPointSize(4);

		globalColorProg.use();
		model.loadIdentity();
		globalColorProg.setUniform(model);

		// Dessiner les lignes séparant les fragments.
		globalColor = vec4(0, 0, 0, 1);
		globalColorProg.setUniform(globalColor);
		gridLines.draw(GL_LINES);

		// Dessiner un point semi-transparent au centre des fragments.
		globalColor = vec4(0, 0, 0, 0.5);
		globalColorProg.setUniform(globalColor);
		gridPoints.draw(GL_POINTS);
	}

	void applyOrtho() {
		float ylim = gridSize;
		float xlim = gridSize;
		// Centrer la grille et les faux fragments au milieu de la fenêtre (plus convenant).
		float xmargin = (getWindowAspect() - 1) * ylim * 0.5f;
		projection.loadIdentity();
		projection.ortho2D(-xmargin, xlim + xmargin, 0, ylim);

		globalColorProg.use();
		globalColorProg.setMat(projection);
		quadGenProg.use();
		quadGenProg.setMat(projection);
	}

	void loadShaders() {
		globalColorProg.create();
		globalColorProg.attachSourceFile(GL_VERTEX_SHADER, "common_vert.glsl");
		globalColorProg.attachSourceFile(GL_FRAGMENT_SHADER, "uniform_frag.glsl");
		globalColorProg.link();

		quadGenProg.create();
		quadGenProg.attachSourceFile(GL_VERTEX_SHADER, "common_vert.glsl");
		quadGenProg.attachSourceFile(GL_GEOMETRY_SHADER, "quad_geom.glsl");
		quadGenProg.attachSourceFile(GL_FRAGMENT_SHADER, "color_frag.glsl");
		quadGenProg.link();

	}
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	settings.context.antiAliasingLevel = 0;

	App app;
	app.run(argc, argv, "Exemple Semaine 10: Algorithmes matériel", settings);
}
