#pragma once


#include <cstddef>
#include <cstdint>

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>


std::string readFile(std::string_view filename) {
	// Ouvrir le fichier
	std::ifstream file(filename.data());
	// Lire et retourner le contenu du fichier
	return (std::stringstream() << file.rdbuf()).str();
}

