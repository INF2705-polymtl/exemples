#pragma once

#ifdef _WIN32
	// Juste pour s'assurer de traiter le code source en UTF-8 sur Windows avec Visual Studio.
	#pragma execution_character_set("utf-8")
#endif


#include <cstddef>
#include <cstdint>

#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#ifdef _WIN32
	#include <Windows.h>
#endif

#include <glbinding/Binding.h>
#include <glbinding/gl/gl.h>
#include <SFML/Window.hpp>


using namespace gl;


// Fonction utilitaire pour convertir une string UTF-8 en sf::String (pas mal juste utilisé pour le titre de fenêtre).
inline sf::String sfStr(std::string_view s) {
	return sf::String::fromUtf8(s.begin(), s.end());
}

// Fonction utilitaire pour obtenir le nom d'une touche. Ce nom va dépendre de la langue de l'OS.
inline void printKeyName(sf::Event::KeyEvent key) {
	std::cout << std::string(sf::Keyboard::getDescription(key.scancode)) << std::endl;
}


// Classe de base pour les application OpenGL. Fait pour nous la création de fenêtre et la gestion des événements.
// On doit en hériter et on peut surcharger init() et drawFrame() pour créer un programme de base.
// Les autres méthodes à surcharcher sont pour la gestion d'événements.
class OpenGLApplication
{
public:
	virtual ~OpenGLApplication() = default;

	const sf::Window& getWindow() const { return window_; }

	void run(int& argc, char* argv[], std::string_view title = "OpenGL Application", sf::VideoMode videoMode = {800, 800}, int fps = 30) {
		argc_ = argc;
		argv_ = argv;

		createWindowAndContext(title, videoMode, fps);
		printGLInfo();
		std::cout << std::endl;

		init(); // À surcharger

		while (window_.isOpen()) {
			drawFrame(); // À surcharger
			window_.display();

			handleEvents();
		}
	}

	// Appelée avant la première trame.
	virtual void init() { }

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	virtual void drawFrame() { }

	// Appelée lors d'une touche de clavier.
	virtual void onKeyEvent(const sf::Event::KeyEvent& key) { }

	// Appelée lorsque la fenêtre se ferme.
	virtual void onClose() { }

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	virtual void onResize(const sf::Event::SizeEvent& size) { }

	// Appelée sur un évènement autre que Closed, Resized ou KeyPressed.
	virtual void onEvent(const sf::Event& event) { }

protected:
	void handleEvents() {
		// Traiter les événements survenus depuis la dernière trame.
		sf::Event event;
		while (window_.pollEvent(event)) {
			using enum sf::Event::EventType;
			switch (event.type) {
			// L'utilisateur a voulu fermer la fenêtre (le X de la fenêtre, Alt+F4 sur Windows, etc.).
			case Closed:
				onClose(); // À surcharger
				window_.close();
				break;
			// Redimensionnement de la fenêtre.
			case Resized:
				glViewport(0, 0, event.size.width, event.size.height);
				onResize(event.size); // À surcharger
				break;
			// Touche appuyée.
			case KeyPressed:
				onKeyEvent(event.key); // À surcharger
				break;
			// Autre événement.
			default:
				onEvent(event); // À surcharger
				break;
			}
		}
	}

	void createWindowAndContext(std::string_view title, sf::VideoMode videoMode, int fps) {
		#ifdef _WIN32
			// Juste pour s'assurer d'avoir le codepage UTF-8 sur Windows avec Visual Studio.
			SetConsoleOutputCP(65001);
			SetConsoleCP(65001);
		#endif

		window_.create(
			videoMode, // Dimensions de fenêtre.
			sfStr(title), // Titre.
			sf::Style::Default, // Style de fenêtre (bordure, boutons X, etc.).
			sf::ContextSettings(24, 8) // 24bit de depth buffer, 8bit de stencil buffer.
		);
		window_.setFramerateLimit(fps);
		window_.setActive(true);

		// On peut donner une « GetProcAddress » venant d'une autre librairie à glbinding.
		// Si on met nullptr, glbinding se débrouille avec sa propre implémentation.
		glbinding::Binding::initialize(nullptr, true);
	}

	void printGLInfo() {
		// Afficher les informations de base de la carte graphique et de la version OpenGL des drivers.
		auto openglVersion = glGetString(GL_VERSION);
		auto openglVendor = glGetString(GL_VENDOR);
		auto openglRenderer = glGetString(GL_RENDERER);
		auto glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
		printf("OpenGL %s\n", openglVersion);
		printf("GPU    %s, %s\n", openglRenderer, openglVendor);
		printf("GLSL   %s\n", glslVersion);
	}

	sf::Window window_;
	int argc_ = 0;
	char** argv_ = nullptr;
};

