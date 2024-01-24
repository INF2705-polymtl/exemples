#pragma once


#include <cstddef>
#include <cstdint>

#include <format>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <inf2705/utils.hpp>

#include "TransformStack.hpp"


using namespace gl;
using namespace glm;


class ShaderProgram
{
public:
	~ShaderProgram() {
		for (auto&& [type, shaderObjects] : shadersByType_) {
			for (auto&& shader : shaderObjects) {
				// glDetachShader et glDeleteShader fonctionnent un peu comme des pointeurs intelligents : Le shader est concrètement supprimé seulement s'il n'est plus attaché à un programme. Sinon, il est marqué pour suppression mais pas supprimé tout de suite.
				glDeleteShader(shader);
				glDetachShader(programObject_, shader);
			}
		}
		glDeleteProgram(programObject_);
		unuse();
	}

	// Le code donné par OpenGL
	GLuint getObject() const { return programObject_; }

	const std::unordered_set<GLuint>& getShaderObjects(GLenum type) const {
		static const std::unordered_set<GLuint> emptyValue;
		auto it = shadersByType_.find(type);
		return it != shadersByType_.end() ? it->second : emptyValue;
	}

	// Demander à OpenGL de créer un programme de nuanceur
	void create() {
		if (programObject_ != 0)
			*this = {};
		programObject_ = glCreateProgram();
	}

	// Associer le contenu du fichier au nuanceur spécifié.
	GLuint attachSourceFile(GLenum type, std::string_view filename) {
		// Créer le nuanceur.
		GLuint shaderObject = glCreateShader(type);
		if (shaderObject == 0)
			return 0;

		// Charger la source et compiler.
		std::string source = readFile(filename);
		auto src = source.c_str();
		glShaderSource(shaderObject, 1, &src, nullptr);
		glCompileShader(shaderObject);

		// Afficher le message d'erreur si applicable.
		int infologLength = 0;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &infologLength);
		if (infologLength > 1) {
			std::string infoLog(infologLength, '\0');
			glGetShaderInfoLog(shaderObject, infologLength, nullptr, infoLog.data());
			std::cout << std::format("Compilation Error in '{}':\n{}", filename, infoLog) << std::endl;
			glDeleteShader(shaderObject);
			return 0;
		}

		// Attacher au programme.
		attachExistingShader(type, shaderObject);

		return shaderObject;
	}

	void attachExistingShader(GLenum type, GLuint shaderObject) {
		// Attacher au programme.
		glAttachShader(programObject_, shaderObject);
		// Avoir un dictionnaire des nuanceurs de chaque type n'est pas nécessaire mais peut aider au débogage.
		shadersByType_[type].insert(shaderObject);
	}

	// Faire l'édition des liens du programme
	bool link() {
		glLinkProgram(programObject_);

		// Afficher le message d'erreur si applicable.
		GLint infologLength = 0;
		glGetProgramiv(programObject_, GL_INFO_LOG_LENGTH, &infologLength);
		if (infologLength > 1) {
			std::string infoLog(infologLength, '\0');
			glGetProgramInfoLog(programObject_, infologLength, nullptr, infoLog.data());
			std::cout << std::format("Link Error in program {}:\n{}", programObject_, infoLog) << std::endl;
			return false;
		}
		return true;
	}

	// Utiliser ce programme comme pipeline graphique
	void use() {
		glUseProgram(programObject_);
	}

	void unuse() {
		glUseProgram(0);
	}

	// Assigner des variables uniformes
	void setBool(std::string_view name, bool val) { setBool(getUniformLocation(name), (GLint)val); }
	void setInt(std::string_view name, int val) { setInt(getUniformLocation(name), (GLint)val); }
	void setUInt(std::string_view name, unsigned val) { setUInt(getUniformLocation(name), (GLuint)val); }
	void setFloat(std::string_view name, float val) { setFloat(getUniformLocation(name), (GLfloat)val); }
	void setVec(std::string_view name, const vec2& val) { setVec(getUniformLocation(name), val); }
	void setVec(std::string_view name, const vec3& val) { setVec(getUniformLocation(name), val); }
	void setVec(std::string_view name, const vec4& val) { setVec(getUniformLocation(name), val); }
	void setVec(std::string_view name, const ivec2& val) { setVec(getUniformLocation(name), val); }
	void setVec(std::string_view name, const ivec3& val) { setVec(getUniformLocation(name), val); }
	void setVec(std::string_view name, const ivec4& val) { setVec(getUniformLocation(name), val); }
	void setVec(std::string_view name, const uvec2& val) { setVec(getUniformLocation(name), val); }
	void setVec(std::string_view name, const uvec3& val) { setVec(getUniformLocation(name), val); }
	void setVec(std::string_view name, const uvec4& val) { setVec(getUniformLocation(name), val); }
	void setMat(std::string_view name, const mat2& val) { setMat(getUniformLocation(name), val); }
	void setMat(std::string_view name, const mat3& val) { setMat(getUniformLocation(name), val); }
	void setMat(std::string_view name, const mat4& val) { setMat(getUniformLocation(name), val); }
	void setMat(std::string_view name, const TransformStack& val) { setMat(name, val.top()); }
	void setBool(GLuint loc, bool val) { glUniform1i(loc, (GLint)val); }
	void setInt(GLuint loc, int val) { glUniform1i(loc, (GLint)val); }
	void setUInt(GLuint loc, unsigned val) { glUniform1ui(loc, (GLuint)val); }
	void setFloat(GLuint loc, float val) { glUniform1f(loc, (GLfloat)val); }
	void setVec(GLuint loc, const vec2& val) { glUniform2fv(loc, 1, glm::value_ptr(val)); }
	void setVec(GLuint loc, const vec3& val) { glUniform3fv(loc, 1, glm::value_ptr(val)); }
	void setVec(GLuint loc, const vec4& val) { glUniform4fv(loc, 1, glm::value_ptr(val)); }
	void setVec(GLuint loc, const ivec2& val) { glUniform2iv(loc, 1, glm::value_ptr(val)); }
	void setVec(GLuint loc, const ivec3& val) { glUniform3iv(loc, 1, glm::value_ptr(val)); }
	void setVec(GLuint loc, const ivec4& val) { glUniform4iv(loc, 1, glm::value_ptr(val)); }
	void setVec(GLuint loc, const uvec2& val) { glUniform2uiv(loc, 1, glm::value_ptr(val)); }
	void setVec(GLuint loc, const uvec3& val) { glUniform3uiv(loc, 1, glm::value_ptr(val)); }
	void setVec(GLuint loc, const uvec4& val) { glUniform4uiv(loc, 1, glm::value_ptr(val)); }
	void setMat(GLuint loc, const mat2& val) { glUniformMatrix2fv(loc, 1, GL_FALSE, glm::value_ptr(val)); }
	void setMat(GLuint loc, const mat3& val) { glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(val)); }
	void setMat(GLuint loc, const mat4& val) { glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(val)); }
	void setMat(GLuint loc, const TransformStack& val) { setMat(loc, val.top()); }

	// Positions
	GLuint getAttribLocation(std::string_view name) const { return glGetAttribLocation(programObject_, name.data()); }
	void setAttribLocation(GLuint index, std::string_view name) { glBindAttribLocation(programObject_, index, name.data()); }
	GLuint getUniformLocation(std::string_view name) { return glGetUniformLocation(programObject_, name.data()); }

private:
	GLuint programObject_ = 0; // Le ID de programme nuanceur.
	std::unordered_map<GLenum, std::unordered_set<GLuint>> shadersByType_; // Les nuanceurs.
};

