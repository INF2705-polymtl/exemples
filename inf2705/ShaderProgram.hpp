#pragma once


#include <cstddef>
#include <cstdint>

#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
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
				glDetachShader(programObject_, shader);
				glDeleteShader(shader);
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
		std::string source = readFile(filename);
		if (source.empty())
			return 0;

		// Créer le nuanceur.
		GLuint shaderObject = glCreateShader(type);
		if (shaderObject == 0)
			return 0;

		// Charger la source et compiler.
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
			return 0;
		}

		// Attacher au programme.
		glAttachShader(programObject_, shaderObject);
		// Avoir un dictionnaire des nuanceurs de chaque type n'est pas nécessaire mais peut aider au débogage.
		shadersByType_[type].insert(shaderObject);

		return shaderObject;
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
	void setBool(std::string_view name, bool val) { setUniform1i(name, (GLint)val); }
	void setInt(std::string_view name, int val) { setUniform1i(name, (GLint)val); }
	void setUInt(std::string_view name, unsigned val) { setUniform1ui(name, (GLuint)val); }
	void setFloat(std::string_view name, float val) { setUniform1f(name, (GLfloat)val); }
	void setVec(std::string_view name, const vec2& val) { setUniform2fv(name, glm::value_ptr(val)); }
	void setVec(std::string_view name, const vec3& val) { setUniform3fv(name, glm::value_ptr(val)); }
	void setVec(std::string_view name, const vec4& val) { setUniform4fv(name, glm::value_ptr(val)); }
	void setVec(std::string_view name, const ivec2& val) { setUniform2iv(name, glm::value_ptr(val)); }
	void setVec(std::string_view name, const ivec3& val) { setUniform3iv(name, glm::value_ptr(val)); }
	void setVec(std::string_view name, const ivec4& val) { setUniform4iv(name, glm::value_ptr(val)); }
	void setVec(std::string_view name, const uvec2& val) { setUniform2uiv(name, glm::value_ptr(val)); }
	void setVec(std::string_view name, const uvec3& val) { setUniform3uiv(name, glm::value_ptr(val)); }
	void setVec(std::string_view name, const uvec4& val) { setUniform4uiv(name, glm::value_ptr(val)); }
	void setMat(std::string_view name, const mat2& val) { setUniformMatrix2fv(name, glm::value_ptr(val)); }
	void setMat(std::string_view name, const mat3& val) { setUniformMatrix3fv(name, glm::value_ptr(val)); }
	void setMat(std::string_view name, const mat4& val) { setUniformMatrix4fv(name, glm::value_ptr(val)); }
	void setMat(std::string_view name, const TransformStack& val) { setMat(name, val.top()); }

	void setUniform1f(std::string_view name, GLfloat v0) { glUniform1f(getUniformLocation(name), v0); }
	void setUniform2f(std::string_view name, GLfloat v0, GLfloat v1) { glUniform2f(getUniformLocation(name), v0, v1); }
	void setUniform3f(std::string_view name, GLfloat v0, GLfloat v1, GLfloat v2) { glUniform3f(getUniformLocation(name), v0, v1, v2); }
	void setUniform4f(std::string_view name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) { glUniform4f(getUniformLocation(name), v0, v1, v2, v3); }
	void setUniform1i(std::string_view name, GLint v0) { glUniform1i(getUniformLocation(name), v0); }
	void setUniform2i(std::string_view name, GLint v0, GLint v1) { glUniform2i(getUniformLocation(name), v0, v1); }
	void setUniform3i(std::string_view name, GLint v0, GLint v1, GLint v2) { glUniform3i(getUniformLocation(name), v0, v1, v2); }
	void setUniform4i(std::string_view name, GLint v0, GLint v1, GLint v2, GLint v3) { glUniform4i(getUniformLocation(name), v0, v1, v2, v3); }
	void setUniform1ui(std::string_view name, GLuint v0) { glUniform1ui(getUniformLocation(name), v0); }
	void setUniform2ui(std::string_view name, GLuint v0, GLuint v1) { glUniform2ui(getUniformLocation(name), v0, v1); }
	void setUniform3ui(std::string_view name, GLuint v0, GLuint v1, GLuint v2) { glUniform3ui(getUniformLocation(name), v0, v1, v2); }
	void setUniform4ui(std::string_view name, GLuint v0, GLuint v1, GLuint v2, GLuint v3) { glUniform4ui(getUniformLocation(name), v0, v1, v2, v3); }
	void setUniform1fv(std::string_view name, const GLfloat* val, GLsizei count = 1) { glUniform1fv(getUniformLocation(name), count, val); }
	void setUniform2fv(std::string_view name, const GLfloat* val, GLsizei count = 1) { glUniform2fv(getUniformLocation(name), count, val); }
	void setUniform3fv(std::string_view name, const GLfloat* val, GLsizei count = 1) { glUniform3fv(getUniformLocation(name), count, val); }
	void setUniform4fv(std::string_view name, const GLfloat* val, GLsizei count = 1) { glUniform4fv(getUniformLocation(name), count, val); }
	void setUniform1iv(std::string_view name, const GLint* val, GLsizei count = 1) { glUniform1iv(getUniformLocation(name), count, val); }
	void setUniform2iv(std::string_view name, const GLint* val, GLsizei count = 1) { glUniform2iv(getUniformLocation(name), count, val); }
	void setUniform3iv(std::string_view name, const GLint* val, GLsizei count = 1) { glUniform3iv(getUniformLocation(name), count, val); }
	void setUniform4iv(std::string_view name, const GLint* val, GLsizei count = 1) { glUniform4iv(getUniformLocation(name), count, val); }
	void setUniform1uiv(std::string_view name, const GLuint* val, GLsizei count = 1) { glUniform1uiv(getUniformLocation(name), count, val); }
	void setUniform2uiv(std::string_view name, const GLuint* val, GLsizei count = 1) { glUniform2uiv(getUniformLocation(name), count, val); }
	void setUniform3uiv(std::string_view name, const GLuint* val, GLsizei count = 1) { glUniform3uiv(getUniformLocation(name), count, val); }
	void setUniform4uiv(std::string_view name, const GLuint* val, GLsizei count = 1) { glUniform4uiv(getUniformLocation(name), count, val); }
	void setUniformMatrix2fv(std::string_view name, const GLfloat* val, GLsizei count = 1) { glUniformMatrix2fv(getUniformLocation(name), count, GL_FALSE, val); }
	void setUniformMatrix3fv(std::string_view name, const GLfloat* val, GLsizei count = 1) { glUniformMatrix3fv(getUniformLocation(name), count, GL_FALSE, val); }
	void setUniformMatrix4fv(std::string_view name, const GLfloat* val, GLsizei count = 1) { glUniformMatrix4fv(getUniformLocation(name), count, GL_FALSE, val); }

	// Positions
	GLuint getAttribLocation(std::string_view name) const { return(glGetAttribLocation(programObject_, name.data())); }
	void setAttribLocation(GLuint index, std::string_view name) { glBindAttribLocation(programObject_, index, name.data()); }
	GLuint getUniformLocation(std::string_view name) { return glGetUniformLocation(programObject_, name.data()); }

private:
	GLuint programObject_ = 0; // Le ID de programme nuanceur.
	std::unordered_map<GLenum, std::unordered_set<GLuint>> shadersByType_; // Les nuanceurs.
};

