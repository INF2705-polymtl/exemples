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


using namespace gl;
using namespace glm;


struct Buffers
{
	float z;
	vec4 color;
	uint8_t stencil;
};

inline std::ostream& operator<< (std::ostream& out, const Buffers& buffers) {
	return out << std::format("{:.2f} | {:.2f} {:.2f} {:.2f} {:.2f} | {}",
		buffers.z,
		buffers.color.r, buffers.color.g, buffers.color.b, buffers.color.a,
		buffers.stencil
	);
}


struct App : public OpenGLApplication
{
	// Appelée avant la première trame.
	void init() override {
		glPointSize(500.0f);

		loadShaders();
		basicProg.use();

		points.vertices = {{}};
		points.setup();
	}

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		if (not firstRun)
			return;
		firstRun = false;

		// Exemples pris d'un ancien examen.

		runTest("a",
			{0.5, {0.8, 0.8, 0.8, 0.3}, 0},
			{0.7, {0.4, 0.4, 0.4, 0.4}, 0},
			[]() {
				glDisable(GL_STENCIL_TEST);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_BLEND);
			}
		);

		runTest("b",
			{0.5, {0.8, 0.8, 0.8, 0.3}, 0},
			{0.7, {0.4, 0.4, 0.4, 0.4}, 0},
			[]() {
				glDisable(GL_STENCIL_TEST);
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_GREATER);
				glDisable(GL_BLEND);
			}
		);

		runTest("c",
			{0.5, {0.8, 0.8, 0.8, 0.3}, 0},
			{0.7, {0.4, 0.4, 0.4, 0.4}, 2},
			[]() {
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_GEQUAL, 1, 3);
				glStencilOp(GL_KEEP, GL_INCR, GL_REPLACE);
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LEQUAL);
				glDisable(GL_BLEND);
			}
		);

		runTest("d",
			{0.5, {0.8, 0.8, 0.8, 0.3}, 0},
			{0.7, {0.4, 0.4, 0.4, 0.4}, 2},
			[]() {
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_LESS, 1, 3);
				glStencilOp(GL_KEEP, GL_INCR, GL_REPLACE);
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_GREATER);
				glDisable(GL_BLEND);
			}
		);

		runTest("e",
			{0.5, {0.8, 0.8, 0.8, 0.3}, 0},
			{0.7, {0.4, 0.4, 0.4, 0.4}, 2},
			[]() {
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_GREATER, 1, 3);
				glStencilOp(GL_KEEP, GL_INCR, GL_REPLACE);
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				glDisable(GL_BLEND);
			}
		);

		runTest("f",
			{0.5, {0.8, 0.8, 0.8, 0.3}, 0},
			{0.7, {0.4, 0.4, 0.4, 0.4}, 0},
			[]() {
				glDisable(GL_STENCIL_TEST);
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_ALWAYS);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
		);

		runTest("g",
			{0.5, {0.8, 0.8, 0.8, 0.3}, 0},
			{0.7, {0.4, 0.4, 0.4, 0.4}, 0},
			[]() {
				glDisable(GL_STENCIL_TEST);
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			}
		);
	}

	void runTest(const std::string& id, const Buffers& frag, const Buffers& init, std::function<void()> fn) {
		// Afficher le nom du test
		std::cout << id << ")" << "\n";

		// Changer les "clear values" des buffers pour les valeurs initiales voulues.
		glClearColor(init.color.r, init.color.g, init.color.b, init.color.a);
		glClearDepthf(init.z);
		glClearStencil(init.stencil);
		// Faire le clear avec les valeurs précédentes. On va ainsi mettre les valeurs initiales qu'on veut dans les buffers.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// Appliquer la config par défaut d'OpenGL manuellement.
		resetDefaultGLConfig();
		// Appliquer les modifications voulues pour le test.
		fn();

		// Passer les valeurs de fragments qu'on veut en variables uniformes. Les attributs de sommets sont ignorés dans les nuanceurs.
		basicProg.use();
		basicProg.setFloat("vertexZ", frag.z);
		basicProg.setVec("vertexColor", frag.color);
		// Dessiner un point au milieu de l'écran suffisamment gros pour couvrir l'écran.
		glPointSize(500.0f);
		points.draw(GL_POINTS);
		// Attendre que les opérations soient compétées.
		glFinish();

		// Lire les valeurs de buffers pour le fragment au milieu de l'écran.
		Buffers result = {};
		glReadPixels(100, 100, 1, 1, GL_RGBA, GL_FLOAT, &result.color);
		glReadPixels(100, 100, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &result.z);
		glReadPixels(100, 100, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &result.stencil);

		// Afficher les buffers.
		std::cout << "  frag    " << frag << "\n"
		          << "  init    " << init << "\n"
		          << "  result  " << result << "\n"
		          << std::endl;
	}

	void resetDefaultGLConfig() {
		glStencilFunc(GL_ALWAYS, 0, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glDepthFunc(GL_LESS);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glClearColor(0, 0, 0, 0);
		glClearDepthf(0);
		glClearStencil(0);
	}

	void loadShaders() {
		basicProg.create();
		basicProg.attachSourceFile(GL_VERTEX_SHADER, "basic_vert.glsl");
		basicProg.attachSourceFile(GL_FRAGMENT_SHADER, "basic_frag.glsl");
		basicProg.link();
	}

	Mesh points;

	ShaderProgram basicProg;

	TransformStack model;
	TransformStack view;
	TransformStack projection;

	bool firstRun = true;
};


int main(int argc, char* argv[]) {
	WindowSettings settings = {};
	settings.fps = 30;
	settings.context.antialiasingLevel = 0;
	settings.videoMode = {200, 200};

	App app;
	app.run(argc, argv, "Exemple Semaine 5: Calculs de tampons", settings);
}
