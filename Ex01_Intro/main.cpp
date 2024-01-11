#include <cstddef>
#include <cstdint>

#include <iostream>
#include <string>

// Ici on utilise glbinding pour résoudre OpenGL et SFML pour la fenêtre.
// Des alternatives pour la résolution des fonctions sont GLUT, GLFW, GLEW.
// SDL est utilisé dans les exemples des notes de cours et dans les labos.
// On peut aussi se servir de Qt pour faire de l'OpenGL, mais c'est lourd comme environnement.
#include <glbinding/Binding.h>
#include <glbinding/gl/gl.h>
#include <SFML/Window.hpp>


using namespace gl;


// Quelques couleurs en format RGB réel (0 à 1).
float grey[] =         {0.5f, 0.5f, 0.5f};
float white[] =        {1.0f, 1.0f, 1.0f};
float brightRed[] =    {1.0f, 0.2f, 0.2f};
float brightGreen[] =  {0.2f, 1.0f, 0.2f};
float brightBlue[] =   {0.2f, 0.2f, 1.0f};
float brightYellow[] = {1.0f, 1.0f, 0.2f};


void drawWhiteTriangle() {
	// glColor configure la couleur pour les sommets qui suivent.
	// glBegin/glEnd tracent les primitives.
	// glVertex ajoute les sommets.
	// Ces fonctions sont du OpenGL 2.x qu'on ne va plus utiliser pour le reste de la session. Ça fait toutefois une bonne introduction intuitive aux primitives.

	glColor3fv(white);
	glBegin(GL_TRIANGLES); {
		glVertex3f( 0.0f,  0.5f, 0.0f);
		glVertex3f(-0.5f, -0.5f, 0.0f);
		glVertex3f( 0.5f, -0.5f, 0.0f);
	} glEnd();
}

void drawColoredTriangle() {
	// Ici, en mettant une couleur avant chaque sommet on peut observer l'interpolation des attributs de sommet.
	glBegin(GL_TRIANGLES); {
		glColor3fv(brightRed);
		glVertex3f( 0.0f,  0.5f,  0.0f);
		glColor3fv(brightGreen);
		glVertex3f(-0.5f, -0.5f,  0.0f);
		glColor3fv(brightBlue);
		glVertex3f( 0.5f, -0.5f,  0.0f);
	} glEnd();
}

void drawPyramid() {
	// On va s'éviter de la répétition de code en se déclarant des variables pour nos sommets.
	static float top[] =     { 0.0f,  0.2f,  0.0f};
	static float bottom1[] = {-0.3f, -0.2f, -0.1f};
	static float bottom2[] = { 0.3f, -0.2f, -0.1f};
	static float bottom3[] = { 0.0f, -0.2f,  0.7f};

	// Il faut faire attention à la cohérence de l'ordre de traçage des sommets.
	// Par défaut, les faces "avant" sont les faces dessinées en sens antihoraire (configurable avec glFrontFace).
	// Ici, on s'arrange pour que l'avant des faces soit celles visibles de l'extérieur de la forme.
	glBegin(GL_TRIANGLES); {
		// Dessous
		glColor3fv(grey);
		glVertex3fv(bottom1);
		glVertex3fv(bottom2);
		glVertex3fv(bottom3);

		// Face "tribord"
		glColor3fv(brightGreen);
		glVertex3fv(bottom1);
		glVertex3fv(bottom3);
		glVertex3fv(top);

		// Face "babord"
		glColor3fv(brightRed);
		glVertex3fv(bottom3);
		glVertex3fv(bottom2);
		glVertex3fv(top);

		// Face arrière
		glColor3fv(brightBlue);
		glVertex3fv(bottom2);
		glVertex3fv(bottom1);
		glVertex3fv(top);
	} glEnd();
}


int main(int argc, char* argv[]) {
	// Créer la fenêtre et initialiser le contexte OpenGL.
	sf::Window window;
	window.create(
		{800, 800},             // Dimensions de la fenêtre
		"Exemple Semaine 1",    // Titre
		sf::Style::Default,     // Style de fenêtre (avec barre de titre et boutons).
		sf::ContextSettings(24) // Taille du tampon de profondeur. 24 bit est pas mal le standard.
	);
	window.setFramerateLimit(30); // Le FPS d'affichage.
	window.setActive(true);
	glbinding::Binding::initialize(nullptr, true); // La résolution des fonctions d'OpenGL.

	// La caméra qui nous permet de visualiser la profondeur avec une perspective.
	// les détails ne sont pas importants pour aujourd'hui.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-2./3, 2./3, -2./3, 2./3, 4, 8);
	glTranslatef(0, 0, -6);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// On veut que la profondeur soit appliquée.
	glEnable(GL_DEPTH_TEST);

	// La couleur de fond.
	glClearColor(0.05f, 0.1f, 0.1f, 1.0f);

	// Les tailles et types de points/lignes (en pixels).
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(5.0f);
	glEnable(GL_POINT_SMOOTH);
	glPointSize(5.0f);

	// On peut jouer avec le 2e param de glPolygonMode pour changer le type de rendu.
	// Les options sont GL_POINT, GL_LINE, GL_FILL.
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// On peut changer si les faces avant ou arrière sont rendues à l'écran.
	// "Cull" = pas affiché. donc glCullFace(GL_BACK) n'affiche pas les faces arrières (la config par défaut).
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Boucle d'exécution
	while (window.isOpen()) {
		// On efface ce qui a dans le tampon.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Commentez/décommentez pour tester la rotation.
		glRotatef(1, 0, 1, 0);

		// Commentez/décommentez chacune des lignes pour tester chaque affichage.
		//drawWhiteTriangle();
		//drawColoredTriangle();
		drawPyramid();

		// SFML fait le rafraîchissement de la fenêtre ainsi que le contrôle du framerate pour nous.
		window.display();
		// On pourrait gérer les évènements de fenêtre (clavier, redimensionnement, fermeture, etc).
		sf::Event e;
		while (window.pollEvent(e)) {
			// On arrête si l'utilisateur appuie sur le X de la fenêtre.
			if (e.type == sf::Event::EventType::Closed)
				window.close();
		}
	}
}

