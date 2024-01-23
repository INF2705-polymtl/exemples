#include <cstddef>
#include <cstdint>

#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#define GLM_FORCE_SWIZZLE // Pour utiliser les v.xz, v.rb, etc. comme dans GLSL.
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <inf2705/OpenGLApplication.hpp>
#include <inf2705/utils.hpp>


using namespace gl;
using namespace glm;


// Des couleurs
vec4 grey =         {0.5f, 0.5f, 0.5f, 1.0f};
vec4 white =        {1.0f, 1.0f, 1.0f, 1.0f};
vec4 brightRed =    {1.0f, 0.2f, 0.2f, 1.0f};
vec4 brightGreen =  {0.2f, 1.0f, 0.2f, 1.0f};
vec4 brightBlue =   {0.2f, 0.2f, 1.0f, 1.0f};
vec4 brightYellow = {1.0f, 1.0f, 0.2f, 1.0f};


struct App : public OpenGLApplication
{
	void init() override {
		// Config de contexte OpenGL assez de base.
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_POINT_SMOOTH);
		glPointSize(5.0f);
		glLineWidth(2.0f);
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);

		loadShaders();
		initBuffers();
		setupSquareCamera();
	}

	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);

		applyRotation();
		applyBrightness();
		drawPyramid();
	}

	void initBuffers() {
		// Créer le VAO
		glGenVertexArrays(1, &pyramidVao);
		glBindVertexArray(pyramidVao);

		if (usingSingleDataBuffer) {
			initCombinedBuffer();
		} else {
			initPositionBuffer();
			initColorBuffer();
		}

		if (usingElements)
			initElementBuffer();
	}

	void initPositionBuffer() {
		// Créer le VBO des positions.
		glGenBuffers(1, &pyramidPositionVbo);
		// Les glBindBuffers sont enregistrés dans le contexte du VAO.
		glBindBuffer(GL_ARRAY_BUFFER, pyramidPositionVbo);
		// Charger les données dans le tampon.
		if (usingElements)
			glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVertexPositions), pyramidVertexPositions, GL_STATIC_DRAW);
		else
			glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidFullPositions), pyramidFullPositions, GL_STATIC_DRAW);
		// Configurer l'attribut à la position 0 avec 3 float (vec3, donc xyz) par élément.
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);
	}

	void initColorBuffer() {
		// Créer le VBO des couleurs.
		glGenBuffers(1, &pyramidColorVbo);
		glBindBuffer(GL_ARRAY_BUFFER, pyramidColorVbo);
		// Charger les données dans le tampon.
		if (usingElements)
			glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVertexColors), pyramidVertexColors, GL_STATIC_DRAW);
		else
			glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidFullColors), pyramidFullColors, GL_STATIC_DRAW);
		// Configurer l'attribut à la position 1 avec 4 float (vec4, donc rgba) par élément.
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);
	}

	void initElementBuffer() {
		// Créer le VBO des indices de sommet.
		glGenBuffers(1, &pyramidEbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pyramidEbo);
		// Charger les données dans le tampon.
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(pyramidIndices), pyramidIndices, GL_STATIC_DRAW);
	}

	void initCombinedBuffer() {
		// Créer le VBO de données combinées.
		glGenBuffers(1, &pyramidDataVbo);
		glBindBuffer(GL_ARRAY_BUFFER, pyramidDataVbo);
		// Charger les données combinées dans un seul tableau.
		glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVertices), pyramidVertices, GL_STATIC_DRAW);
		// Configurer l'attribut 0 (la position) avec l'index 0 avec 3 float (vec3), un stride de sizeof(Data) et un offset de 0.
		glVertexAttribPointer(
			0,
			3,
			GL_FLOAT,
			GL_FALSE,
			sizeof(Data),
			0
		);
		glEnableVertexAttribArray(0);
		// Configurer l'attribut 1 (la couleur) avec l'index 0 avec 4 float (vec4), un stride de sizeof(Data) et un offset de 3*4=12.
		glVertexAttribPointer(
			1,
			4,
			GL_FLOAT,
			GL_FALSE,
			sizeof(Data),
			(void*)(3*sizeof(float))
		);
		glEnableVertexAttribArray(1);
	}

	void loadShaders() {
		// Créer les objets de shaders.
		shaderProgram = glCreateProgram();
		vertexShader = glCreateShader(GL_VERTEX_SHADER);
		fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

		// Lire et envoyer la source du nuanceur de sommets.
		std::string vertexShaderSource = readFile("pyramid_vert.glsl");
		auto src = vertexShaderSource.c_str();
		glShaderSource(vertexShader, 1, &src, nullptr);
		// Compiler et attacher le nuanceur de sommets.
		glCompileShader(vertexShader);
		glAttachShader(shaderProgram, vertexShader);

		// Lire et envoyer la source du nuanceur de fragments.
		std::string fragmentShaderSource = readFile("pyramid_frag.glsl");
		src = fragmentShaderSource.c_str();
		glShaderSource(fragmentShader, 1, &src, nullptr);
		// Compiler et attacher le nuanceur de fragments.
		glCompileShader(fragmentShader);
		glAttachShader(shaderProgram, fragmentShader);

		// Lier les deux nuanceurs au pipeline du programme.
		glLinkProgram(shaderProgram);
	}

	void setupSquareCamera() {
		// Configuration de caméra qui donne une projection perspective où le plan z=0 a une boîte -1 à 1 en x, y.
		// Ça ressemble donc à la projection par défaut (orthogonale dans une boîte -1 à 1) mais avec une perspective.

		// La matrice de visualisation positionne la caméra dans l'espace.
		view = glm::lookAt(vec3{0.0f, 0.0f, 6.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
		// La matrice de projection contrôle comment la caméra observe la scène.
		projection = glm::frustum(-2.0f/3, 2.0f/3, -2.0f/3, 2.0f/3, 4.0f, 8.0f);
		// Ça peut paraître redondant, mais il faut faire glUseProgram avant se faire des glUniform* même si ceux-ci utilisent déjà le ID du programme dans l'obtention de l'adresse de la variable uniforme.
		// Les adresses de variables données par glGetUniformLocation ne sont pas globalement uniques et il faut se mettre dans le contexte du programme spécifique.
		glUseProgram(shaderProgram);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));
	}

	void applyRotation() {
		model = glm::rotate(model, radians(1.0f), {0.0f, 1.0f, 0.0f});
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, value_ptr(model));
	}

	void applyBrightness() {
		// Exemple de variable uniforme utilisé par le nuanceur de fragment, mais inutile au nuanceur de sommets.
		brightness += 0.01f;
		glUniform1f(glGetUniformLocation(shaderProgram, "brightness"), brightness);
	}

	void drawPyramid() {
		glBindVertexArray(pyramidVao);
		// Comme mentionné ci dessus, les glBindBuffers sont enregistrés dans le contexte du VAO. En faisant glBindVertexArray, ça restaure les glBindBuffer, on n'a donc pas besoin de les refaire.

		if (usingElements)
			// glDrawElements utilise un tableau d'indices pour prendre les données des array buffers. Chaque sommet de la pyramide est partagé par 3 faces. On met donc les données de chaque sommet une seule fois en mémoire et on y fait référence à travers un tableau de connectivité.
			glDrawElements(GL_TRIANGLES, (GLint)std::size(pyramidIndices), GL_UNSIGNED_INT, 0);
		else
			glDrawArrays(GL_TRIANGLES, 0, (GLint)std::size(pyramidFullPositions));
	}

	// Détermine si on utilise l'exemple avec glDrawArrays ou glDrawElements.
	bool usingElements = true;
	// Détermine si on utilise le buffer de données combinées.
	bool usingSingleDataBuffer = true;

	// Les données de la pyramide
	vec3 top =     { 0.0f,  0.2f,  0.0f};
	vec3 bottomR = {-0.3f, -0.2f, -0.1f};
	vec3 bottomL = { 0.3f, -0.2f, -0.1f};
	vec3 bottomF = { 0.0f, -0.2f,  0.7f};
	vec3 pyramidVertexPositions[4] = {
		top,
		bottomR,
		bottomL,
		bottomF,
	};
	vec4 pyramidVertexColors[4] = {
		brightYellow,
		brightGreen,
		brightRed,
		brightBlue,
	};
	GLuint pyramidIndices[4 * 3] = {
		// Dessous
		1, 2, 3,
		// Face "tribord"
		1, 3, 0,
		// Face "babord"
		3, 2, 0,
		// Face arrière
		2, 1, 0,
	};
	vec3 pyramidFullPositions[4 * 3] = {
		// Dessous
		bottomR, bottomL, bottomF,
		// Face "tribord"
		bottomR, bottomF, top,
		// Face "babord"
		bottomF, bottomL, top,
		// Face arrière
		bottomL, bottomR, top,
	};
	vec4 pyramidFullColors[4 * 3] = {
		// Dessous
		brightGreen, brightRed, brightBlue,
		// Face "tribord"
		brightGreen, brightBlue, brightYellow,
		// Face "babord"
		brightBlue, brightRed, brightYellow,
		// Face arrière
		brightRed, brightGreen, brightYellow,
	};
	struct Data
	{
		vec3 position;
		vec4 color;
	};
	Data pyramidVertices[4] = {
		{top, brightYellow},
		{bottomR, brightGreen},
		{bottomL, brightRed},
		{bottomF, brightBlue},
	};

	// Les ID d'objets OpenGL
	GLuint pyramidVao = 0;
	GLuint pyramidPositionVbo = 0;
	GLuint pyramidColorVbo = 0;
	GLuint pyramidEbo = 0;
	GLuint pyramidDataVbo = 0;

	// Les nuanceurs
	GLuint shaderProgram = 0;
	GLuint vertexShader = 0;
	GLuint fragmentShader = 0;

	// Les matrices de tranformation
	mat4 model = mat4(1.0f);
	mat4 view = mat4(1.0f);
	mat4 projection = mat4(1.0f);

	float brightness = 1.0f;
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};

	App app;
	app.run(argc, argv, "Exemple Semaine 3: Transformations", settings);
}


///////////////////////////////////////////////////////////////////////////////
// Dompe de codes qui illustrent certains éléments
///////////////////////////////////////////////////////////////////////////////

// Illustrer les valeurs de glUniformLocation.
/*
auto sourceVert = "#version 410 \n uniform float uni1, uni2; \n void main() { gl_Position = vec4(uni1, uni2, 0, 1); } \n";
auto sourceFrag = "#version 410 \n void main() { } \n";

GLuint p1 = glCreateProgram();
GLuint s1v = glCreateShader(GL_VERTEX_SHADER);
GLuint s1f = glCreateShader(GL_FRAGMENT_SHADER);

glShaderSource(s1v, 1, &sourceVert, nullptr);
glCompileShader(s1v);
glAttachShader(p1, s1v);

glShaderSource(s1f, 1, &sourceFrag, nullptr);
glCompileShader(s1f);
glAttachShader(p1, s1f);

glLinkProgram(p1);
glUseProgram(p1);

GLuint p2 = glCreateProgram();
GLuint s2v = glCreateShader(GL_VERTEX_SHADER);
GLuint s2f = glCreateShader(GL_FRAGMENT_SHADER);

glShaderSource(s2v, 1, &sourceVert, nullptr);
glCompileShader(s2v);
glAttachShader(p2, s2v);

glShaderSource(s2f, 1, &sourceFrag, nullptr);
glCompileShader(s2f);
glAttachShader(p2, s2f);

glLinkProgram(p2);
glUseProgram(p2);

GLuint u11 = glGetUniformLocation(p1, "uni1");
GLuint u12 = glGetUniformLocation(p1, "uni2");
GLuint u21 = glGetUniformLocation(p2, "uni1");
GLuint u22 = glGetUniformLocation(p2, "uni2");
*/

// Illustrer le buffer mapping
/*
	void onKeyEvent(const sf::Event::KeyEvent& key) override {
		float positionMod = 0.0f;
		float colorMod = 0.0f;

		using enum sf::Keyboard::Key;
		switch (key.code) {
		case Up:
			positionMod = 0.1f;
			break;
		case Down:
			positionMod = -0.1f;
			break;
		case Add:
			colorMod = 0.1f;
			break;
		case Subtract:
			colorMod = -0.1f;
			break;
		}

		glBindVertexArray(pyramidVao);
		auto ptr = (Data*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
		auto& top = ptr[0];
		top.position.y += positionMod;
		top.color.rgb = top.color.rgb + vec3(colorMod);
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
*/

// Illustrer le subdata
/*
	void onKeyEvent(const sf::Event::KeyEvent& key) override {
		float positionMod = 0.0f;
		float colorMod = 0.0f;

		using enum sf::Keyboard::Key;
		switch (key.code) {
		case Up:
			positionMod = 0.1f;
			break;
		case Down:
			positionMod = -0.1f;
			break;
		case Add:
			colorMod = 0.1f;
			break;
		case Subtract:
			colorMod = -0.1f;
			break;
		}

		glBindVertexArray(pyramidVao);
		auto& top = pyramidVertices[0];
		top.position.y += positionMod;
		top.color.rgb = top.color.rgb + vec3(colorMod);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Data), &top);
	}
*/

