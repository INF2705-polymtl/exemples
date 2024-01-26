#pragma once


#include <cstddef>
#include <cstdint>

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "TransformStack.hpp"


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

	void applyView(TransformStack& viewMatrix) const {
		viewMatrix.translate({0, 0, -altitude});
		viewMatrix.rotate(roll, {0, 0, 1});
		viewMatrix.rotate(latitude, {1, 0, 0});
		viewMatrix.rotate(longitude, {0, 1, 0});
	}
};

