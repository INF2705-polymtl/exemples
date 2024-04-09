#include <cstddef>
#include <cstdint>

#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <thread>

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


struct Particle
{
	vec3 position;
	vec3 velocity;
	float mass;
	float miscValue;

	static void setAttribs() {
		SET_VEC_VERTEX_ATTRIB_FROM_STRUCT_MEM(0, Particle, position);
		SET_VEC_VERTEX_ATTRIB_FROM_STRUCT_MEM(1, Particle, velocity);
		SET_SCALAR_VERTEX_ATTRIB_FROM_STRUCT_MEM(2, Particle, mass);
		SET_SCALAR_VERTEX_ATTRIB_FROM_STRUCT_MEM(3, Particle, miscValue);
	}
};

struct App : public OpenGLApplication
{
	std::vector<Particle> particles;
	GLuint vaoDrawing = 0;
	GLuint vaoComputation = 0;
	GLuint vboIn = 0;
	GLuint vboOut = 0;
	GLuint tfoComputation = 0;
	GLuint reqParticles = 0;

	ShaderProgram computationProg;
	ShaderProgram drawingProg;

	TransformStack model = {"model"};
	TransformStack view = {"view"};
	TransformStack projection = {"projection"};

	Uniform<float> deltaTime = {"deltaTime"};
	Uniform<vec3> forceFieldPosition = {"forceFieldPosition"};
	Uniform<float> forceFieldStrength = {"forceFieldStrength"};
	Uniform<float> speedMax = {"speedMax"};
	Uniform<float> globalSpeedFactor = {"globalSpeedFactor", 1};

	float orthoHeight = 50;
	bool savingData = false;

	// Appelée avant la première trame.
	void init() override {
		// Vu que l'affichage et le traitement vont être un peu plus lourd, on active le cull et désactive le blend et z-test.
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);

		loadShaders();

		constexpr size_t numParticles = 100'000;
		particles.resize(numParticles);

		// Générer l'état initial des particules. On applique une distribution normale (gaussienne) centrée en 0,0 pour la position et la vitesse et une distribution uniforme pour la masse.
		std::mt19937_64 randEng(time(nullptr));
		std::normal_distribution<float> normDist(0.0f, 2.0f);
		std::uniform_real_distribution<float> uniDist(0.5f, 1.0f);
		auto randomNorm = [&]() { return normDist(randEng); };
		auto randomUni = [&]() { return uniDist(randEng); };
		for (size_t i = 0; i < numParticles; i++) {
			Particle p = {};
			// Position avec distribution normale.
			p.position = {randomNorm(), randomNorm(), 0};
			// Vitesse = position donc se déplace initialement de façon radiale à l'origine.
			p.velocity = p.position;
			// Masse distribuée uniformément.
			p.mass = randomUni();
			// Valeur unique pour chaque particule (exemple d'autre valeur qu'on peut passer aux sommets).
			p.miscValue = (float)i / (numParticles - 1);
			particles[i] = p;
		}
		// Mélanger les particules.
		std::shuffle(particles.begin(), particles.end(), randEng);

		// Créer les VAO. Dans notre exemple assez simple, ce n'est pas nécessaire d'en avoir deux. Mais en général, on va avoir un VAO pour chaque mesh à afficher, donc ceux-ci auront très probablement des configs différentes du VAO de calcul (EBO différent, vertex attribs venant de plusieurs VBO, etc.). Bref n'importe quel état qui est sauvegardé dans le VAO et pourrait être différent pour le calcul et l'affichage.
		glGenVertexArrays(1, &vaoDrawing);
		glGenVertexArrays(1, &vaoComputation);
		// Créer les VBO.
		glGenBuffers(1, &vboIn);
		glGenBuffers(1, &vboOut);
		// Créer la requête et le TFO (Transform Feedback Object).
		glGenQueries(1, &reqParticles);
		glGenTransformFeedbacks(1, &tfoComputation);

		// Passer les données des particles dans le VBO d'entrée. On configure avec GL_DYNAMIC_COPY vu que ça va être souvent lu et modifié. Ce n'est qu'une suggestion au driver et celui-ci peut l'ignorer.
		auto numBytes = (GLsizeiptr)particles.size() * sizeof(Particle);
		glBindBuffer(GL_ARRAY_BUFFER, vboIn);
		glBufferData(GL_ARRAY_BUFFER, numBytes, particles.data(), GL_DYNAMIC_COPY);
		// Configurer le VBO de sortie avec le même nombre d'octets. On n'a pas besoin de passer des données vu qu'il va être rempli avec les résultats de calculs.
		glBindBuffer(GL_ARRAY_BUFFER, vboOut);
		glBufferData(GL_ARRAY_BUFFER, numBytes, nullptr, GL_DYNAMIC_COPY);

		// Lier le TFO au VAO de calcul (pas besoin pour le VAO d'affichage).
		glBindVertexArray(vaoComputation);
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfoComputation);

		// Configurer les variables de sortie de la rétroaction. Il faut passer les noms dans l'ordre dans lequel ils seront écrits dans le tampon de sortie. Dans notre cas, on met le même ordre que dans la struct `Particle`. On veut que la sortie des calculs ait le même format que les données en entrée.
		const char* variableNames[] = {
			"position",
			"velocity",
			"mass",
			"miscValue",
		};
		glTransformFeedbackVaryings(
			computationProg.getObject(),
			(GLsizei)std::size(variableNames),
			variableNames,
			GL_INTERLEAVED_ATTRIBS
		);
		// glTransformFeedbackVaryings doit être appelée AVANT l'édition de lien du programme de nuanceurs.
		computationProg.link();

		// La vitesse max des particules.
		speedMax = 20;
		computationProg.setUniform(speedMax);
		drawingProg.setUniform(speedMax);

		applyOrtho();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// Exécuter le pipeline de calcul. On utilise vboIn comme source de données et les résultats sont mis dans vboOut.
		computePhysics();

		// Échanger les VBO. De cette façon, le programme d'affichage peut utiliser les résultats de calcul comme données d'entrée, d'où l'idée d'avoir le même format de données dans les deux VBO.
		std::swap(vboIn, vboOut);

		// Afficher les particules avec le programme d'affichage qui a un nuanceur de géométrie donnant une forme aux particules.
		drawParticles();

		if (savingData) {
			// Pour sauvegarder, il faut attendre que les commandes soient exécutées, puis faire la requête pour rapatrier les données sur le CPU.
			glFinish();
			queryParticleData();
			// Enregistrer les données dans un fichier CSV avec un nom généré.
			saveParticleDataAsCsv();
			// Prendre une capture d'écran tant qu'à y être et la mettre dans le même dossier que le CSV.
			saveScreenshot("output");

			savingData = false;
		}
	}

	// Appelée lors d'une touche de clavier.
	void onKeyPress(const sf::Event::KeyEvent& key) override {
		// La touche R réinitialise la position de la caméra.
		// Les flèches bougent la caméra dans le plan XY.
		// Espace fait freiner les particules.
		// F sauvegarde les données de particules dans un fichier en plus d'un screenshot.

		float translation = orthoHeight * 0.05f;
		using enum sf::Keyboard::Key;
		switch (key.code) {
		case R:
			view.loadIdentity();
			orthoHeight = 50;
			break;
		case Up:
			view.translate({0, -translation, 0});
			break;
		case Down:
			view.translate({0, translation, 0});
			break;
		case Left:
			view.translate({translation, 0, 0});
			break;
		case Right:
			view.translate({-translation, 0, 0});
			break;

		case Space:
			globalSpeedFactor = 0.9f;
			computationProg.setUniform(globalSpeedFactor);
			break;

		case F:
		case F5:
			savingData = true;
			break;
		}

		drawingProg.setUniform(view);
		applyOrtho();
	}

	// Appelée lors d'une touche de clavier relachée.
	void onKeyRelease(const sf::Event::KeyEvent& key) override {
		using enum sf::Keyboard::Key;
		switch (key.code) {
		case Space:
			globalSpeedFactor = 1;
			computationProg.setUniform(globalSpeedFactor);
			break;
		}
	}

	// Appelée lors d'un bouton de souris appuyé.
	void onMouseButtonPress(const sf::Event::MouseButtonEvent& mouseBtn) override {
		// Bouton gauche appuyé : champs attractif
		// Bouton droit appuyé : champs répulsif
		auto& mouse = getMouse();
		// Mettre à jour l'intensité du champs de force selon les boutons de la souris.
		computationProg.setUniform(forceFieldStrength);
		if (mouseBtn.button == sf::Mouse::Left)
			forceFieldStrength = 10;
		if (mouseBtn.button == sf::Mouse::Right)
			forceFieldStrength = -10;
		computationProg.setUniform(forceFieldStrength);
	}

	// Appelée lors d'un bouton de souris relâché.
	void onMouseButtonRelease(const sf::Event::MouseButtonEvent& mouseBtn) override {
		// Relâcher le bouton de souris désactive le champs de force.
		if (mouseBtn.button == sf::Mouse::Left)
			forceFieldStrength = 0;
		else if (mouseBtn.button == sf::Mouse::Right)
			forceFieldStrength = 0;
		computationProg.setUniform(forceFieldStrength);
	}

	// Appelée lors d'un défilement de souris.
	void onMouseScroll(const sf::Event::MouseWheelScrollEvent& mouseScroll) override {
		// Zoom in/out
		orthoHeight = std::max(orthoHeight * (1 - 0.1f * mouseScroll.delta), 0.1f);
		applyOrtho();
	}

	// Appelée lorsque la fenêtre se redimensionne (juste après le redimensionnement).
	void onResize(const sf::Event::SizeEvent& event) override {
		applyOrtho();
	}

	void computePhysics() {
		computationProg.use();

		// Mettre à jour le temps depuis la dernière trame.
		deltaTime = getFrameDeltaTime();
		computationProg.setUniform(deltaTime);

		// Mettre à jour l'origine du champs de force avec la souris.
		// Pour obtenir la position de la souris dans la scène (on a juste une scène 2D dans notre cas donc on assume z=0) on applique l'inverse de la matrice projection * visualisation aux coordonnées d'écrans normarlisées de la souris.
		// En effet, si prend les coordonnées de la souris relatives au centre de la fenêtre et normalisées dans [-1,1], on a l'équivalent des coordonnées normalisées d'OpenGL. En appliquant l'inverse des matrices de proj et visu (pas modèle), on obtient donc les coordonnées de scène.
		// P·V·M·A = A′          où A est un vecteur de position et A′ est sa coords normalisées
		//     M·A = (P·V)⁻¹·A′  or M·A est la coordonnées de scène (ou universelle).
		vec2 mousePosition = vec2(getMouse().normalized.x, getMouse().normalized.y);
		mat4 invTransform = inverse(projection * view);
		auto v = invTransform * vec4(mousePosition, 0, 1);
		forceFieldPosition = vec3(v);
		computationProg.setUniform(forceFieldPosition);

		glBindVertexArray(vaoComputation);
		// Configurer le VBO d'entrée pour les données de sommets. Il faut répéter les configurations d'attributs quand on bind un différent de VBO.
		glBindBuffer(GL_ARRAY_BUFFER, vboIn);
		Particle::setAttribs();
		// Configurer le VBO de sortie pour contenir les résultats de calculs.
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vboOut);

		// Commencer la requête et la rétroaction.
		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, reqParticles);
		glBeginTransformFeedback(GL_POINTS);
		// Désactiver le tramage.
		glEnable(GL_RASTERIZER_DISCARD);
		// Dessiner les particles (appel normal).
		glDrawArrays(GL_POINTS, 0, (GLint)particles.size());
		// Réactiver le tramage.
		glDisable(GL_RASTERIZER_DISCARD);
		// Terminer la requête et la rétroaction.
		glEndTransformFeedback();
		glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
	}

	void drawParticles() {
		drawingProg.use();

		// Rien de très spécial ici, on fait le bind et les configs d'attributs puis on dessine.
		glBindVertexArray(vaoDrawing);
		glBindBuffer(GL_ARRAY_BUFFER, vboIn);
		Particle::setAttribs();
		glDrawArrays(GL_POINTS, 0, (GLsizei)particles.size());
	}

	void queryParticleData() {
		GLuint numResults = 0;
		// Obtenir le nombre de résultats capturés par la requête.
		glGetQueryObjectuiv(reqParticles, GL_QUERY_RESULT, &numResults);
		// Si ce nombre est non-nul, on a des résultats.
		if (numResults != 0) {
			// Lire le contenu du tampon de rétroaction dans le tableau de particules original (donc le mettre à jour).
			auto numBytes = particles.size() * sizeof(Particle);
			glGetBufferSubData(
				GL_TRANSFORM_FEEDBACK_BUFFER,
				0,
				numBytes,
				particles.data()
			);
		}
	}

	void saveParticleDataAsCsv() {
		// Construire un nom de fichier avec l'heure de départ de l'application et le numéro de trame.
		std::string dateTimeStr = formatStartTime("%Y%m%d_%H%M%S");
		std::string filename = std::format(
			"output/particles_{}_{}.csv",
			dateTimeStr,
			getCurrentFrameNumber()
		);
		auto dataCopy = particles;

		// Dans notre exemple, on peut facilement gérer des millions de particules. Formater et écrire sur le disque autant de données peut prendre plusieurs secondes. On fait donc cette écriture dans un fil d'exécution (thread) séparé en lui  une copie des données de particules. Ça permet de moins ralentir le fil principal qui fait l'affichage.
		// On note la capture par copie des variables `filename` et `dataCopy`.
		std::thread savingThread([=]() {
			std::ofstream file(filename);
			file << "m\tx\ty\tvx\tvy" << "\n";
			for (auto& p : dataCopy) {
				file << std::format(
					"{:.7e}\t{:.7e}\t{:.7e}\t{:.7e}\t{:.7e}",
					p.mass,
					p.position.x, p.position.y,
					p.velocity.x, p.velocity.y
				) << "\n";
			}
			// Pour aider au débogage, afficher le contenu du fichier dans la sortie standard si peu de données.
			if (dataCopy.size() <= 10) {
				file.close();
				std::cout << readFile(filename) << "\n";
			}
		});
		savingThread.detach();
	}

	void applyOrtho() {
		// Projection orthogonale proportionnelle aux proportions de la fenêtre.
		float aspect = getWindowAspect();
		projection.ortho(
			-orthoHeight / 2 * aspect,
			orthoHeight / 2 * aspect,
			-orthoHeight / 2,
			orthoHeight / 2,
			-1,
			1
		);
		drawingProg.setUniform(projection);
	}

	void loadShaders() {
		drawingProg.attachSourceFile(GL_VERTEX_SHADER, "particles_draw_vert.glsl");
		drawingProg.attachSourceFile(GL_GEOMETRY_SHADER, "particles_draw_geom.glsl");
		drawingProg.attachSourceFile(GL_FRAGMENT_SHADER, "color_frag.glsl");
		drawingProg.link();

		computationProg.attachSourceFile(GL_VERTEX_SHADER, "particles_compute_vert.glsl");
		// L'édition de lien (le linking) pour le prog de calcul est faite plus tard après avoir fait glTransformFeedbackVaryings.
	}
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	settings.context.antialiasingLevel = 4;

	App app;
	app.run(argc, argv, "Exemple Semaine 9: Rétroaction avec VBO", settings);
}
