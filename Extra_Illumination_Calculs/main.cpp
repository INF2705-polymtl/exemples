#include <cstddef>
#include <cstdint>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <inf2705/TransformStack.hpp>


using namespace gl;
using namespace glm;


float computeSpecularPhong(vec3 l, vec3 n, vec3 o, float shininess) {
	return pow(std::max(dot(reflect(-l, n), o), 0.0f), shininess);
}

float computeSpecularBlinn(vec3 l, vec3 n, vec3 o, float shininess) {
	return pow(std::max(dot(normalize(l + o), n), 0.0f), shininess);
}

void runLightingModelComputations(const std::string& testName, vec3 posL, vec3 posO, vec3 n1, vec3 n2, vec3 p1, vec3 p2, float shininess) {
	n1 = normalize(n1);
	n2 = normalize(n2);
	vec3 l1 = posL - p1;
	vec3 l2 = posL - p2;
	vec3 o1 = posO - p1;
	vec3 o2 = posO - p2;

	std::vector<float> xs;
	for (float x = p1.x; x <= p2.x; x += 0.1f)
		xs.push_back(x);

	// Avec Gouraud, on calcule les couleurs aux sommets et on interpole dans les fragments.
	std::vector<float> gouraudDiff, gouraudSpec, gouraudSpecBlinn;
	float diff1 = dot(n1, normalize(l1));
	float diff2 = dot(n2, normalize(l2));
	float spec1 = computeSpecularPhong(normalize(l1), n1, normalize(o1), shininess);
	float spec2 = computeSpecularPhong(normalize(l2), n2, normalize(o2), shininess);
	float specBlinn1 = computeSpecularBlinn(normalize(l1), n1, normalize(o1), shininess);
	float specBlinn2 = computeSpecularBlinn(normalize(l2), n2, normalize(o2), shininess);
	for (float x : xs) {
		float xValue = (x - p1.x) / (p2.x - p1.x);
		float diffuse = mix(diff1, diff2, xValue);
		float specular = mix(spec1, spec2, xValue);
		float specularBlinn = mix(specBlinn1, specBlinn2, xValue);
		gouraudDiff.push_back(std::clamp(diffuse, 0.0f, 1.0f));
		gouraudSpec.push_back(std::clamp(specular, 0.0f, 1.0f));
		gouraudSpecBlinn.push_back(std::clamp(specularBlinn, 0.0f, 1.0f));
	}

	// Avec Phong, on calcule les vecteurs (L, N et O) aux sommets et on calcule les couleurs dans les fragments avec les vecteurs interpolés.
	std::vector<float> phongDiff, phongSpec, phongSpecBlinn;
	for (float x : xs) {
		float xValue = (x - p1.x) / (p2.x - p1.x);
		vec3 l = normalize(mix(l1, l2, xValue));
		vec3 n = normalize(mix(n1, n2, xValue));
		vec3 o = normalize(mix(o1, o2, xValue));
		float diff = dot(n, l);
		float spec = computeSpecularPhong(l, n, o, shininess);
		float specBlinn = computeSpecularBlinn(l, n, o, shininess);
		phongDiff.push_back(std::clamp(diff, 0.0f, 1.0f));
		phongSpec.push_back(std::clamp(spec, 0.0f, 1.0f));
		phongSpecBlinn.push_back(std::clamp(specBlinn, 0.0f, 1.0f));
	}

	std::ofstream fileDiff("output/" + testName + "_diff.csv");
	std::ofstream fileSpec("output/" + testName + "_spec.csv");
	std::ofstream fileSpecBlinn("output/" + testName + "_spec_blinn.csv");
	fileDiff << "x\tGouraud\tPhong" << "\n";
	fileSpec << "x\tGouraud\tPhong" << "\n";
	fileSpecBlinn << "x\tGouraud\tPhong" << "\n";
	for (size_t i = 0; i < xs.size(); i++) {
		fileDiff << std::format("{}\t{}\t{}\n", xs[i], gouraudDiff[i], phongDiff[i]);
		fileSpec << std::format("{}\t{}\t{}\n", xs[i], gouraudSpec[i], phongSpec[i]);
		fileSpecBlinn << std::format("{}\t{}\t{}\n", xs[i], gouraudSpecBlinn[i], phongSpecBlinn[i]);
	}
}


int main(int argc, char* argv[]) {
	std::filesystem::create_directory("output");

	// Vous pouvez utiliser cette fonction pour générer des courbes d'interpolations selon les modèles de Gouraud et Phong. Les données sont ensuite importables dans Excel pour la visualisation.
	runLightingModelComputations(
		"test1",
		{0, 4, 0},
		{0, 4, 0},
		{0, 1, 0},
		{1, 1, 0},
		{-4, 0, 0},
		{ 4, 0, 0},
		50
	);
	runLightingModelComputations(
		"test2",
		{0, 4, 0},
		{0, 4, 0},
		{0, 1, 0},
		{1, 2, 0},
		{-4, 0, 0},
		{4, 0, 0},
		50
	);
}
