#pragma once


#include <cstddef>
#include <cstdint>

#include <array>
#include <ctime>
#include <format>
#include <iostream>
#include <memory>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <chrono>
#include <unordered_map>
#include <thread>

#ifdef _WIN32
	#include <Windows.h>
#endif

#include <glbinding/Binding.h>
#include <glbinding/gl/gl.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "sfml_utils.hpp"


using namespace gl;


struct WindowSettings
{
	sf::VideoMode videoMode = {600, 600};
	int fps = 30;
	sf::ContextSettings context = sf::ContextSettings(24, 8);
};

// Classe de base pour les application OpenGL. Fait pour nous la création de fenêtre et la gestion des événements.
// On doit en hériter et on peut surcharger init() et drawFrame() pour créer un programme de base.
// Les autres méthodes à surcharger sont pour la gestion d'événements.
class OpenGLApplication
{
public:
	virtual ~OpenGLApplication() = default;

	const sf::Window& getWindow() const { return window_; }

	void run(int& argc, char* argv[], std::string_view title = "OpenGL Application", const WindowSettings& settings = {}) {
		argc_ = argc;
		argv_ = argv;
		settings_ = settings;

		startTime_ = std::chrono::system_clock::now();

		createWindowAndContext(title);
		printGLInfo();
		std::cout << std::endl;

		init(); // À surcharger

		lastFrameTime_ = std::chrono::high_resolution_clock::now();
		deltaTime_ = 1.0f / settings_.fps;
		currentMouseState_ = lastMouseState_ = getMouseState(window_);

		while (window_.isOpen()) {
			drawFrame(); // À surcharger
			window_.display();

			handleEvents();
			updateDeltaTime();
			frame_++;
		}
	}

	void printGLInfo() {
		// Afficher les informations de base de la carte graphique et de la version OpenGL des drivers.
		auto openglVersion = glGetString(GL_VERSION);
		auto openglVendor = glGetString(GL_VENDOR);
		auto openglRenderer = glGetString(GL_RENDERER);
		auto glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
		auto& sfmlSettings = window_.getSettings();
		printf("OpenGL       %s\n", openglVersion);
		printf("GPU          %s, %s\n", openglRenderer, openglVendor);
		printf("GLSL         %s\n", glslVersion);
		printf("SFML Context %i.%i\n", sfmlSettings.majorVersion, sfmlSettings.minorVersion);
		printf("Depth bits   %i\n", sfmlSettings.depthBits);
		printf("Stencil bits %i\n", sfmlSettings.stencilBits);
	}

	// Obtenir l'état de la souris (mis à jour une fois par trame avant la gestion d'événements).
	const MouseState& getMouse() const {
		return currentMouseState_;
	}

	int getCurrentFrameNumber() const {
		return frame_;
	}

	float getFrameDeltaTime() const {
		deltaTime_;
	}

	float getWindowAspect() const {
		// Calculer l'aspect de notre caméra à partir des dimensions de la fenêtre.
		auto windowSize = window_.getSize();
		float aspect = (float)windowSize.x / windowSize.y;
		return aspect;
	}

	sf::Image captureCurrentFrame() {
		// Les dimensions de la fenêtre.
		auto windowSize = window_.getSize();
		size_t numPixels = windowSize.x * windowSize.y;

		// Obtenir la source actuelle de glReadBuffer.
		GLint readBufferSrc;
		glGetIntegerv(GL_READ_BUFFER, &readBufferSrc);
		// Lire du front buffer (le tampon d'affichage, donc ce qui est à l'écran). On remarque qu'on n'a pas besoin de faire glFinish(), vu que le tampon d'affichage est complet après le buffer swap.
		glReadBuffer(GL_FRONT);
		std::vector<sf::Uint8> pixels(numPixels * sizeof(sf::Color), 0);
		glReadPixels(0, 0, windowSize.x, windowSize.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		// Restaurer la source de glReadBuffer.
		glReadBuffer((GLenum)readBufferSrc);
		// Créer l'image avec les pixels lus.
		sf::Image img;
		img.create(windowSize.x, windowSize.y, pixels.data());
		// Renverser l'image verticalement à cause de l'origine (x,y=0,0) OpenGL qui est bas-gauche et celle des images SFML qui est haut-gauche.
		img.flipVertically();

		return std::move(img);
	}

	void saveScreenshot(const std::string& folder = "screenshots", const std::string& filename = "") {
		// Capturer la trame actuelle.
		sf::Image frameImage = captureCurrentFrame();

		int frameNumber = frame_;
		std::time_t timestamp = std::chrono::system_clock::to_time_t(startTime_);
		// Faire l'écriture dans le fichier dans un fil parallèle pour moins ralentir le fil principal avec une écriture sur le disque. La capture (avec glReadPixels) doit être faite dans le fil principal, mais l'écriture sur le disque peut être faite en parallèle sans causer de problème de synchronisation. On remarque la capture par copie.
		std::thread thr([=]() {
			using namespace std::filesystem;

			path folderPath(folder);
			if (not folder.empty())
				create_directory(folderPath);

			std::string filePathStr;
			if (not filename.empty()) {
				filePathStr = (folderPath / path(filename)).make_preferred().string();
			} else {
				// Si aucun nom de fichier est fourni, construire un nom avec le nom de l'exécutable, l'heure de démarrage de l'application et le numéro de la trame actuelle.
				path execFilename = path(argv_[0]).stem();
				std::string dateTimeStr(512, 0);
				strftime(dateTimeStr.data(), dateTimeStr.size(), "%Y%m%d_%H%M%S", localtime(&timestamp));
				filePathStr = std::format(
					"{}_{}_{}.png",
					(folderPath / execFilename).make_preferred().string(),
					dateTimeStr.c_str(),
					frameNumber
				);
			}
			frameImage.saveToFile(filePathStr);
		});
		// Détacher le fil pour qu'il se gère tout seul, donc pas besoin de join() ou de garder la variable vivante.
		thr.detach();
	}

	// Appelée avant la première trame.
	virtual void init() { }

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	virtual void drawFrame() { }

	// Appelée lors d'une touche de clavier.
	virtual void onKeyPress(const sf::Event::KeyEvent& key) { }

	// Appelée lors d'une touche de clavier relachée.
	virtual void onKeyRelease(const sf::Event::KeyEvent& key) { }

	// Appelée lors d'un bouton de souris appuyé.
	virtual void onMouseButtonPress(const sf::Event::MouseButtonEvent& mouseBtn) { }

	// Appelée lors d'un bouton de souris relâché.
	virtual void onMouseButtonRelease(const sf::Event::MouseButtonEvent& mouseBtn) { }

	// Appelée lors d'un mouvement de souris.
	virtual void onMouseMove(const sf::Event::MouseMoveEvent& mouseDelta) { }

	// Appelée lors d'un défilement de souris.
	virtual void onMouseScroll(const sf::Event::MouseWheelScrollEvent& mouseScroll) { }

	// Appelée lorsque la fenêtre se ferme.
	virtual void onClose() { }

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	virtual void onResize(const sf::Event::SizeEvent& event) { }

	// Appelée sur un évènement autre que Closed, Resized ou KeyPressed.
	virtual void onEvent(const sf::Event& event) { }

protected:
	void handleEvents() {
		lastMouseState_ = currentMouseState_;
		currentMouseState_ = getMouseState(window_);

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
			case Resized: {
				glViewport(0, 0, event.size.width, event.size.height);
				onResize(event.size); // À surcharger
				lastResize_ = event.size;
				break;
			}
			// Touche appuyée.
			case KeyPressed:
				onKeyPress(event.key); // À surcharger
				break;
			// Touche relâchée.
			case KeyReleased:
				onKeyRelease(event.key); // À surcharger
				break;
			// Bouton appuyé.
			case MouseButtonPressed:
				onMouseButtonPress(event.mouseButton); // À surcharger
				break;
			// Bouton relâché.
			case MouseButtonReleased:
				onMouseButtonRelease(event.mouseButton); // À surcharger
				break;
			// Souris bougée.
			case MouseEntered:
				break;
			case MouseMoved:
				onMouseMove({
					event.mouseMove.x - lastMouseState_.relative.x,
					event.mouseMove.y - lastMouseState_.relative.y
				});
				break;
			// Souris défilée
			case MouseWheelScrolled:
				onMouseScroll(event.mouseWheelScroll);
				break;
			// Autre événement.
			default:
				onEvent(event); // À surcharger
				break;
			}
		}
	}

	void createWindowAndContext(std::string_view title) {
		#ifdef _WIN32
			// Juste pour s'assurer d'avoir le codepage UTF-8 sur Windows avec Visual Studio.
			SetConsoleOutputCP(65001);
			SetConsoleCP(65001);
		#endif

		window_.create(
			settings_.videoMode, // Dimensions de fenêtre.
			sfStr(title), // Titre.
			sf::Style::Default, // Style de fenêtre (bordure, boutons X, etc.).
			settings_.context
		);
		window_.setFramerateLimit(settings_.fps);
		window_.setActive(true);
		lastResize_ = {window_.getSize().x, window_.getSize().y};

		// On peut donner une « GetProcAddress » venant d'une autre librairie à glbinding.
		// Si on met nullptr, glbinding se débrouille avec sa propre implémentation.
		glbinding::Binding::initialize(nullptr, true);
	}

	void updateDeltaTime() {
		using namespace std::chrono;
		auto t = high_resolution_clock::now();
		duration<float> dt = t - lastFrameTime_;
		deltaTime_ = dt.count();
		lastFrameTime_ = t;
	}

	sf::Window window_;
	sf::Event::SizeEvent lastResize_ = {};
	int frame_ = 0;
	float deltaTime_ = 0.0f;
	std::chrono::system_clock::time_point startTime_;
	std::chrono::high_resolution_clock::time_point lastFrameTime_;
	MouseState lastMouseState_ = {};
	MouseState currentMouseState_ = {};

	int argc_ = 0;
	char** argv_ = nullptr;
	WindowSettings settings_;
};


void printGLError(std::string_view sourceFile = "", int sourceLine = -1) {
	static const std::unordered_map<GLenum, std::string> codeToName = {
		{GL_NO_ERROR, "GL_NO_ERROR"},
		{GL_INVALID_ENUM, "GL_INVALID_ENUM"},
		{GL_INVALID_VALUE, "GL_INVALID_VALUE"},
		{GL_INVALID_OPERATION, "GL_INVALID_OPERATION"},
		{GL_STACK_OVERFLOW, "GL_STACK_OVERFLOW"},
		{GL_STACK_UNDERFLOW, "GL_STACK_UNDERFLOW"},
		{GL_OUT_OF_MEMORY, "GL_OUT_OF_MEMORY"},
		{GL_INVALID_FRAMEBUFFER_OPERATION, "GL_INVALID_FRAMEBUFFER_OPERATION"},
		{GL_INVALID_FRAMEBUFFER_OPERATION_EXT, "GL_INVALID_FRAMEBUFFER_OPERATION_EXT"},
		{GL_TABLE_TOO_LARGE, "GL_TABLE_TOO_LARGE"},
		{GL_TABLE_TOO_LARGE_EXT, "GL_TABLE_TOO_LARGE_EXT"},
		{GL_TEXTURE_TOO_LARGE_EXT, "GL_TEXTURE_TOO_LARGE_EXT"},
	};

	if (not sourceFile.empty()) {
		auto filename = std::filesystem::path(sourceFile).filename().string();
		std::cerr << std::format(
			"{}({}): ",
			filename, sourceLine
		);
	}

	while (true) {
		GLenum errorCode = glGetError();
		if (errorCode == GL_NO_ERROR)
			break;

		auto& errorName = codeToName.at(errorCode);
		std::cerr << std::format(
			"OpenGL Error 0x{:04X}: {}\n",
			(int)errorCode, errorName.data()
		);
	}
}

