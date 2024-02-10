#pragma once


#include <cstddef>
#include <cstdint>

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "TransformStack.hpp"
#include "ShaderProgram.hpp"
#include "sfml_utils.hpp"


using namespace glm;


struct OrbitCamera
{
	float altitude = 5;
	float latitude = 0;
	float longitude = 0;
	float roll = 0;

	void moveNorth(float angleDegrees) { latitude += angleDegrees; }
	void moveSouth(float angleDegrees) { latitude -= angleDegrees; }
	void moveWest(float angleDegrees) { longitude += angleDegrees; }
	void moveEast(float angleDegrees) { longitude -= angleDegrees; }
	void rollCW(float angleDegrees) { roll += angleDegrees; }
	void rollCCW(float angleDegrees) { roll -= angleDegrees; }

	void handleKeyEvent(const sf::Event::KeyEvent& key, float angleStep, float distanceStep, OrbitCamera reset = {}) {
		// La touche R réinitialise la position de la caméra.
		// Les touches + et - rapprochent et éloignent la caméra orbitale.
		// Les touches haut/bas change l'élévation ou la latitude de la caméra orbitale.
		// Les touches gauche/droite change la longitude ou le roulement (avec shift) de la caméra orbitale.
		using enum sf::Keyboard::Key;
		switch (key.code) {
		case R:
			*this = reset;
			break;
		case Add:
			altitude -= distanceStep;
			break;
		case Subtract:
			altitude += distanceStep;
			break;
		case Up:
			moveNorth(angleStep);
			break;
		case Down:
			moveSouth(angleStep);
			break;
		case Left:
			if (key.shift)
				rollCCW(angleStep);
			else
				moveWest(angleStep);
			break;
		case Right:
			if (key.shift)
				rollCW(angleStep);
			else
				moveEast(angleStep);
			break;
		}
	}

	void handleMouseMoveEvent(sf::Event::MouseMoveEvent move, const MouseState& mouse, float degsPerPixel = 1.0f) {
		if (mouse.buttons[sf::Mouse::Middle] and mouse.isInsideWindow) {
			float deltaLong = std::clamp(move.x * degsPerPixel, -degsPerPixel * 20, degsPerPixel * 20);
			float deltaLat =  std::clamp(move.y * degsPerPixel, -degsPerPixel * 20, degsPerPixel * 20);
			moveNorth(deltaLat);
			moveWest(deltaLong);
		}
	}

	void applyToView(TransformStack& viewMatrix) const {
		// L'ordre des opération est important. Il faut se rappeler que de modifier la caméra est l'inverse de modifier la scène au complet. 
		viewMatrix.translate({0, 0, -altitude});
		viewMatrix.rotate(roll, {0, 0, 1});
		viewMatrix.rotate(latitude, {1, 0, 0});
		viewMatrix.rotate(longitude, {0, 1, 0});
	}

	void updateProgram(ShaderProgram& prog, std::string_view uniformName, TransformStack& viewMatrix) {
		prog.use();
		updateProgram(prog, prog.getUniformLocation(uniformName), viewMatrix);
	}

	void updateProgram(ShaderProgram& prog, GLuint uniformLoc, TransformStack& viewMatrix) {
		viewMatrix.push(); {
			prog.use();
			applyToView(viewMatrix);
			// En positionnant la caméra, on met seulement à jour la matrice de visualisation.
			prog.setMat(uniformLoc, viewMatrix);
		} viewMatrix.pop();
	}
};

