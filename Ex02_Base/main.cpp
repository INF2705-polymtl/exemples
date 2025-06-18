#include <cstddef>
#include <cstdint>

#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <glbinding/Binding.h>
#include <glbinding/gl/gl.h>
#include <SFML/Window.hpp>

#include <inf2705/utils.hpp>


using namespace gl;


GLuint shaderProgram = 0;
GLuint vertexShader = 0;
GLuint fragmentShader = 0;

void initShaders() {
	// Créer les objets de shaders.
	shaderProgram = glCreateProgram();
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Lire et envoyer la source du nuanceur de sommets.
	std::string vertexShaderSource = readFile("vert.glsl");
	auto src = vertexShaderSource.c_str();
	glShaderSource(vertexShader, 1, &src, nullptr);
	// Compiler et attacher le nuanceur de sommets.
	glCompileShader(vertexShader);
	glAttachShader(shaderProgram, vertexShader);
	// Lire et envoyer la source du nuanceur de fragments.
	std::string fragmentShaderSource = readFile("frag.glsl");
	src = fragmentShaderSource.c_str();
	glShaderSource(fragmentShader, 1, &src, nullptr);
	// Compiler et attacher le nuanceur de fragments.
	glCompileShader(fragmentShader);
	glAttachShader(shaderProgram, fragmentShader);

	// Lier les deux nuanceurs au pipeline du programme.
	glLinkProgram(shaderProgram);
}


int main(int argc, char* argv[]) {
		// Créer la fenêtre et initialiser le contexte OpenGL.
	sf::Window window;
	window.create(
		{800, 800},                // Dimensions de la fenêtre
		"Exemple Semaine 2: Base", // Titre
		sf::Style::Default,        // Style de fenêtre (avec barre de titre et boutons).
		sf::ContextSettings(24)    // Taille du tampon de profondeur. 24 bit est pas mal le standard.
	);
	window.setFramerateLimit(30); // Le FPS d'affichage.
	window.setActive(true);
	glbinding::Binding::initialize(nullptr, true); // La résolution des fonctions d'OpenGL.

	// La couleur de fond
	glClearColor(0.1f, 0.2f, 0.2f, 1.0f);

	// Les détails des nuanceurs ne sont pas importants pour cet exemple.
	initShaders();

	// Les objets (des ints représentant des identifiants) de VAO et VBO
	GLuint vao, vbo[2];
	// Les données de position XYZ pour chaque sommet
	GLfloat xyz[] = {
		 0.0f,  0.5f,  0.0f, // Haut
		-0.5f, -0.5f,  0.0f, // Bas-gauche
		 0.5f, -0.5f,  0.0f, // Bas-droit
	};
	// Les données de couleur RGBA pour chaque sommet
	GLfloat rgba[] = {
		1.0f, 0.0f, 0.0f, 1.0f, // Haut
		0.0f, 1.0f, 0.0f, 1.0f, // Bas-gauche
		0.0f, 0.0f, 1.0f, 1.0f, // Bas-droit
	};
	glGenVertexArrays(1, &vao);
	glGenBuffers(2, vbo);
	glBindVertexArray(vao); // ce vao conservera les informations qui suivent
	// Chargement des positions XYZ de sommets dans le premier VBO
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(xyz), xyz, GL_STATIC_DRAW);
	// On établit les position comme premier attribut (no 0, 3 float par sommet).
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// Chargement des couleurs RGBA de sommets dans le deuxième VBO.
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rgba), rgba, GL_STATIC_DRAW);
	// On établit les position comme deuxième attribut (no 1,43 float par sommet).
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindVertexArray(0); // on termine avec ce vao

	glUseProgram(shaderProgram);

	// Boucle d'exécution
	while (window.isOpen()) {
		// On efface ce qui a dans le tampon.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Activer le VAO
		glBindVertexArray(vao);
		// Dessiner le triangles (3 sommets).
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindVertexArray(0);

		// SFML fait le rafraîchissement de la fenêtre ainsi que le contrôle du framerate pour nous.
		// La fonction display fait le buffer swap (comme glutSwapBuffers) et attend à la prochaine trame selon le FPS qu'on a spécifié avec setFramerateLimit.
		window.display();

		// On pourrait gérer les évènements de fenêtre (clavier, redimensionnement, fermeture, etc), mais pour aujourd'hui on va juste traiter l'événement de fermeture de fenêtre.
		sf::Event e;
		while (window.pollEvent(e)) {
			// On arrête si l'utilisateur appuie sur le X de la fenêtre (ou le raccourci clavier associé tel que Alt+F4 sur Windows).
			if (e.type == sf::Event::EventType::Closed)
				window.close();
		}
	}
}

