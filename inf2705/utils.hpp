#pragma once


#include <cstddef>
#include <cstdint>

#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <type_traits>

#include <glbinding/gl/enum.h>


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

template <typename T>
inline constexpr GLenum getTypeGLenum() {
	if constexpr (std::is_same_v<T, GLbyte>)
		return GL_BYTE;
	else if constexpr (std::is_same_v<T, GLubyte>)
		return GL_UNSIGNED_BYTE;
	else if constexpr (std::is_same_v<T, GLshort>)
		return GL_SHORT;
	else if constexpr (std::is_same_v<T, GLushort>)
		return GL_UNSIGNED_SHORT;
	else if constexpr (std::is_same_v<T, GLint>)
		return GL_INT;
	else if constexpr (std::is_same_v<T, GLuint>)
		return GL_UNSIGNED_INT;
	else if constexpr (std::is_same_v<T, GLfloat>)
		return GL_FLOAT;
	else if constexpr (std::is_same_v<T, GLdouble>)
		return GL_DOUBLE;
	else
		return GL_INVALID_ENUM;
}

template <typename T>
constexpr GLenum getTypeGLenum_v = getTypeGLenum<T>();

template <typename T1, typename T2, typename... Ts>
inline constexpr bool isTypeOneOf() {
	if constexpr (sizeof...(Ts) > 0)
		return std::is_same_v<T1, T2> or isTypeOneOf<T1, Ts...>();
	else
		return std::is_same_v<T1, T2>;
}

template <typename T1, typename T2, typename... Ts>
constexpr bool isTypeOneOf_v = isTypeOneOf<T1, T2, Ts...>();

