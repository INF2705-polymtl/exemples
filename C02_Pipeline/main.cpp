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
#include <inf2705/utils.hpp>


using namespace gl;
using namespace glm;


// Des couleurs
const vec4 grey =   {0.5f, 0.5f, 0.5f, 1.0f};
const vec4 white =  {1.0f, 1.0f, 1.0f, 1.0f};
const vec4 red =    {1.0f, 0.2f, 0.2f, 1.0f};
const vec4 green =  {0.2f, 1.0f, 0.2f, 1.0f};
const vec4 blue =   {0.2f, 0.2f, 1.0f, 1.0f};
const vec4 yellow = {1.0f, 1.0f, 0.2f, 1.0f};


struct App : public OpenGLApplication
{
	// Détermine si on utilise l'exemple avec glDrawArrays ou glDrawElements.
	bool usingElements = true;
	// Détermine si on utilise le buffer de données combinées.
	bool usingSingleDataBuffer = true;

	// Les positions des sommets (pour éviter la répétition de code).
	vec3 top = {0.0f,  0.2f,  0.0f};
	vec3 bottomR = {-0.3f, -0.2f, -0.1f};
	vec3 bottomL = {0.3f, -0.2f, -0.1f};
	vec3 bottomF = {0.0f, -0.2f,  0.7f};
	// Le tableau de positions uniques (à utiliser avec tableau de connectivité).
	vec3 pyramidVertexPositions[4] = {
		top,
		bottomR,
		bottomL,
		bottomF,
	};
	// Le tableau de couleurs uniques (à utiliser avec tableau de connectivité).
	vec4 pyramidVertexColors[4] = {
		yellow,
		green,
		red,
		blue,
	};
	// Le tableau de connectivité.
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
	// Le tableau complet de positions (face par face, donc sans connectivité).
	vec3 pyramidAllPositions[4 * 3] = {
		// Dessous
		bottomR, bottomL, bottomF,
		// Face "tribord"
		bottomR, bottomF, top,
		// Face "babord"
		bottomF, bottomL, top,
		// Face arrière
		bottomL, bottomR, top,
	};
	// Le tableau complet des couleurs (face par face, donc sans connectivité).
	vec4 pyramidAllColors[4 * 3] = {
		// Dessous
		green, red, blue,
		// Face "tribord"
		green, blue, yellow,
		// Face "babord"
		blue, red, yellow,
		// Face arrière
		red, green, yellow,
	};
	// Struct pour combiner les positions et couleurs de façon contigüe.
	struct VertexData
	{
		vec3 position;
		vec4 color;
	};
	// Le tableau de données combinées uniques (à utiliser avec tableau de connectivité).
	VertexData pyramidVertices[4] = {
		{top, yellow},
		{bottomR, green},
		{bottomL, red},
		{bottomF, blue},
	};
	// Le tableau complet de données combinées (face par face, donc sans connectivité).
	VertexData pyramidAllVertices[4 * 3] = {
		// Dessous
		{bottomR, green}, {bottomL, red}, {bottomF, blue},
		// Face "tribord"
		{bottomR, green}, {bottomF, blue}, {top, yellow},
		// Face "babord"
		{bottomF, blue}, {bottomL, red}, {top, yellow},
		// Face arrière
		{bottomL, red}, {bottomR, green}, {top, yellow},
	};

	// Les ID d'objets OpenGL
	GLuint pyramidVao = 0;
	GLuint pyramidPositionVbo = 0;
	GLuint pyramidColorVbo = 0;
	GLuint pyramidCombinedDataVbo = 0;
	GLuint pyramidEbo = 0;

	// Les nuanceurs
	GLuint shaderProgram = 0;
	GLuint vertexShader = 0;
	GLuint fragmentShader = 0;

	// Les matrices de transformation
	mat4 model = mat4(1.0f);
	mat4 view = mat4(1.0f);
	mat4 projection = mat4(1.0f);

	float brightness = 1.0f;

	// Appelée avant la première trame.
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

		// Créer le VAO
		glGenVertexArrays(1, &pyramidVao);
		glBindVertexArray(pyramidVao);

		// Remplir les tampons selon si on veut utiliser des tampons séparés pour les couleurs et positions ou avoir les infos contigües dans le même tampon.
		if (usingSingleDataBuffer) {
			initCombinedBuffer();
		} else {
			initPositionBuffer();
			initColorBuffer();
		}

		if (usingElements)
			initElementBuffer();

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
		// Afficher le message d'erreur si applicable.
		GLint infologLength = 0;
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &infologLength);
		if (infologLength > 1) {
			std::string infoLog(infologLength, '\0');
			glGetShaderInfoLog(vertexShader, infologLength, nullptr, infoLog.data());
			std::cerr << std::format("Compilation Error in '{}':\n{}", "pyramid_vert.glsl", infoLog) << std::endl;
			glDeleteShader(vertexShader);
			return;
		}

		// Lire et envoyer la source du nuanceur de fragments.
		// Vous pouvez changer pour "pyramid_brightness_frag.glsl" pour observer l'effet de la variable uniforme brightness sur le nuanceur de fragment.
		std::string fragmentShaderSource = readFile("pyramid_frag.glsl");
		src = fragmentShaderSource.c_str();
		glShaderSource(fragmentShader, 1, &src, nullptr);
		// Compiler et attacher le nuanceur de fragments.
		glCompileShader(fragmentShader);
		glAttachShader(shaderProgram, fragmentShader);
		// Afficher le message d'erreur si applicable.
		infologLength = 0;
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &infologLength);
		if (infologLength > 1) {
			std::string infoLog(infologLength, '\0');
			glGetShaderInfoLog(fragmentShader, infologLength, nullptr, infoLog.data());
			std::cerr << std::format("Compilation Error in '{}':\n{}", "pyramid_frag.glsl", infoLog) << std::endl;
			glDeleteShader(fragmentShader);
			return;
		}

		// Lier les deux nuanceurs au pipeline du programme.
		glLinkProgram(shaderProgram);

		// Configurer la "caméra" pour cadrer les dessins dans la fenêtre. Les détails mathématiques ne sont pas importants pour aujourd'hui.
		setupSquareCamera();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);

		// Faire tourner la pyramide sur elle-même en appliquant une rotation à chaque trame. On ne réinitialise pas la matrice à chaque trame, donc les rotations s'accumulent (d'où la rotation continue).
		model = glm::rotate(model, radians(1.0f), {0.0f, 1.0f, 0.0f});
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, value_ptr(model));

		// Exemple de variable uniforme utilisée par le nuanceur de fragment, mais inutile au nuanceur de sommets. On incrémente la luminosité à chaque trame et on met à jour la variable uniforme.
		brightness += 0.01f;
		glUniform1f(glGetUniformLocation(shaderProgram, "brightness"), brightness);

		glBindVertexArray(pyramidVao);
		// Comme mentionné ci-dessus, les glBindBuffers sont enregistrés dans le contexte du VAO. En faisant glBindVertexArray, ça restaure les glBindBuffer, on n'a donc pas besoin de les refaire.

		if (usingElements)
			// glDrawElements utilise un tableau d'indices pour prendre les données des array buffers. Chaque sommet de la pyramide est partagé par 3 faces. On met donc les données de chaque sommet une seule fois en mémoire et on y fait référence à travers un tableau de connectivité.
			glDrawElements(GL_TRIANGLES, (GLint)std::size(pyramidIndices), GL_UNSIGNED_INT, 0);
		else
			glDrawArrays(GL_TRIANGLES, 0, (GLint)std::size(pyramidIndices));
	}

	// Appelée lorsque la fenêtre se ferme.
	void onClose() override {
		glDeleteVertexArrays(1, &pyramidVao);
		glDeleteBuffers(1, &pyramidPositionVbo);
		glDeleteBuffers(1, &pyramidColorVbo);
		glDeleteBuffers(1, &pyramidCombinedDataVbo);
		glDeleteBuffers(1, &pyramidEbo);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		glDeleteProgram(shaderProgram);
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
			glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidAllPositions), pyramidAllPositions, GL_STATIC_DRAW);
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
			glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidAllColors), pyramidAllColors, GL_STATIC_DRAW);
		// Configurer l'attribut à la position 1 avec 4 float (vec4, donc RGBA) par élément.
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
		glGenBuffers(1, &pyramidCombinedDataVbo);
		glBindBuffer(GL_ARRAY_BUFFER, pyramidCombinedDataVbo);

		// Charger les données combinées dans un seul tableau. Encore là, on choisit le tableau minimal ou complet selon l'utilisation d'un tableau de connectivité.
		if (usingElements)
			glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVertices), pyramidVertices, GL_STATIC_DRAW);
		else
			glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidAllVertices), pyramidAllVertices, GL_STATIC_DRAW);

		// Configurer l'attribut 0 (la position) à l'indice 0 avec 3 float (vec3), un stride de sizeof(VertexData) et un offset de 0.
		glVertexAttribPointer(
			0,
			3,
			GL_FLOAT,
			GL_FALSE,
			sizeof(VertexData),
			0 // Ou offsetof(VertexData, position)
		);
		glEnableVertexAttribArray(0);
		// Configurer l'attribut 1 (la couleur) à l'indice 0 avec 4 float (vec4), un stride de sizeof(VertexData) et un offset de 3*4=12.
		glVertexAttribPointer(
			1,
			4,
			GL_FLOAT,
			GL_FALSE,
			sizeof(VertexData),
			(void*)(3 * sizeof(float)) // Ou offsetof(VertexData, color)
		);
		glEnableVertexAttribArray(1);
	}

	void setupSquareCamera() {
		// Configuration de caméra qui donne une projection perspective où le plan z=0 a une boîte -1 à 1 en x, y.
		// Ça ressemble donc à la projection par défaut (orthogonale dans une boîte -1 à 1) mais avec une perspective.

		// La matrice de visualisation positionne la caméra dans l'espace.
		view = glm::lookAt(vec3{0.0f, 0.0f, 6.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
		// La matrice de projection contrôle comment la caméra observe la scène.
		projection = glm::frustum(-2.0f / 3, 2.0f / 3, -2.0f / 3, 2.0f / 3, 4.0f, 8.0f);
		// Ça peut paraître redondant, mais il faut faire glUseProgram avant de faire des glUniform* même si ceux-ci utilisent déjà le ID du programme dans l'obtention de l'adresse de la variable uniforme.
		// Les adresses de variables données par glGetUniformLocation ne sont pas globalement uniques et il faut se mettre dans le contexte du programme spécifique.
		glUseProgram(shaderProgram);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));
	}
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};

	App app;
	app.run(argc, argv, "Exemple Semaine 2: Pipeline programmable", settings);
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
	void onKeyPress(const sf::Event::KeyPressed& key) override {
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
		auto ptr = (VertexData*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
		auto& top = ptr[0];
		top.position.y += positionMod;
		top.color.rgb = top.color.rgb + vec3(colorMod);
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
*/

// Illustrer le subdata
/*
	void onKeyPress(const sf::Event::KeyPressed& key) override {
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
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(VertexData), &top);
	}
*/

