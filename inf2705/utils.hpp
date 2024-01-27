#pragma once


#include <cstddef>
#include <cstdint>

#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>


inline std::string readFile(std::string_view filename) {
	// Ouvrir le fichier
	std::ifstream file(filename.data());
	// Lire et retourner le contenu du fichier
	return (std::stringstream() << file.rdbuf()).str();
}

inline std::string ltrim(std::string_view str) {
	if (str.empty())
		return "";
	size_t i;
	for (i = 0; i < str.size() and iswspace(str[i]); i++) { }
	return std::string(str.begin() + i, str.end());
}

inline std::string rtrim(std::string_view str) {
	if (str.empty())
		return "";
	size_t i;
	for (i = str.size() - 1; i >= 0 and iswspace(str[i]); i--) { }
	return std::string(str.begin(), str.begin() + i + 1);
}

inline std::string trim(std::string_view str) {
	return ltrim(rtrim(str));
}

