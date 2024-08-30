#pragma once


#include <cstddef>
#include <cstdint>

#include <format>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils.hpp"
#include "TransformStack.hpp"


using namespace gl;
using namespace glm;


template <typename T>
class Uniform;


class ShaderProgram
{
public:
	ShaderProgram() = default;

	ShaderProgram(GLuint obj) : programObject_(obj) { }

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

	// Associer le contenu d'un fichier au nuanceur spécifié.
	GLuint attachSourceFile(GLenum type, std::string_view filename) {
		if (programObject_ == 0)
			create();

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
		GLint infologLength = 0;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &infologLength);
		if (infologLength > 1) {
			std::string infoLog(infologLength, '\0');
			glGetShaderInfoLog(shaderObject, infologLength, nullptr, infoLog.data());
			std::cerr << std::format("Compilation Error in '{}':\n{}", filename, infoLog) << std::endl;
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
			std::cerr << std::format("Link Error in program {}:\n{}", programObject_, infoLog) << std::endl;
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

	void deleteShaders() {
		for (auto&& [type, shaderObjects] : shadersByType_) {
			for (auto&& shader : shaderObjects) {
				// glDetachShader et glDeleteShader fonctionnent un peu comme des pointeurs intelligents : Le shader est concrètement supprimé seulement s'il n'est plus attaché à un programme. Sinon, il est marqué pour suppression mais pas supprimé tout de suite.
				glDeleteShader(shader);
				glDetachShader(programObject_, shader);
			}
		}
		shadersByType_.clear();
	}

	void deleteProgram() {
		glDeleteProgram(programObject_);
		programObject_ = 0;
		unuse();
	}

	// Assigner des variables uniformes
	void setBool(std::string_view name, bool val) { setBool(getUniformLocation(name), (GLint)val); }
	void setInt(std::string_view name, int val) { setInt(getUniformLocation(name), (GLint)val); }
	void setUint(std::string_view name, unsigned val) { setUint(getUniformLocation(name), (GLuint)val); }
	void setFloat(std::string_view name, float val) { setFloat(getUniformLocation(name), (GLfloat)val); }
	void setTextureUnit(std::string_view name, int val) { setInt(name, val); }
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
	void setUint(GLuint loc, unsigned val) { glUniform1ui(loc, (GLuint)val); }
	void setFloat(GLuint loc, float val) { glUniform1f(loc, (GLfloat)val); }
	void setTextureUnit(GLuint loc, int val) { setInt(loc, val); }
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
	void setMat(const TransformStack& val) { setMat(val.getLoc(getObject()), val.top()); }
	void setMat(TransformStack& val) { setMat(val.getLoc(getObject()), val.top()); }

	// Variable uniforme générique
	template <typename T>
	void setUniform(GLuint loc, const T& val) {
		// Du beau C++ pour choisir à la compilation quelle méthode choisir pour mettre à jour une variable uniforme selon le type de valeur.
		if constexpr (isTypeOneOf_v<T, vec2, vec3, vec4, ivec2, ivec3, ivec4, uvec2, uvec3, uvec4>) {
			setVec(loc, val);
		} else if constexpr (isTypeOneOf_v<T, mat2, mat3, mat4, TransformStack>) {
			setMat(loc, val);
		} else if constexpr (std::is_same_v<T, bool>) {
			setBool(loc, (bool)val);
		} else if constexpr (std::is_integral_v<T>) {
			if constexpr (std::is_signed_v<T>)
				setInt(loc, (int)val);
			else
				setUint(loc, (unsigned)val);
		} else if constexpr (std::is_floating_point_v<T>) {
			setFloat(loc, (float)val);
		}
	}

	template <typename T>
	void setUniform(std::string_view name, const T& val) {
		setUniform(getUniformLocation(name), val);
	}

	template <typename T>
	void setUniform(Uniform<T>& uniValue) {
		setUniform(uniValue.getLoc(*this), uniValue.get());
	}

	void setUniform(const TransformStack& val) {
		setMat(val);
	}

	void setUniform(TransformStack& val) {
		setMat(val);
	}

	void bindUniformBlock(std::string_view name, GLuint bindingIndex) {
		glUniformBlockBinding(programObject_, getUniformBlockIndex(name), bindingIndex);
	}

	void bindUniformBlock(GLuint blockIndex, GLuint bindingIndex) {
		glUniformBlockBinding(programObject_, blockIndex, bindingIndex);
	}

	// Positions
	GLuint getAttribLocation(std::string_view name) const {
		return glGetAttribLocation(programObject_, name.data());
	}

	void setAttribLocation(GLuint index, std::string_view name) {
		glBindAttribLocation(programObject_, index, name.data());
	}

	GLuint getUniformLocation(std::string_view name) const {
		return glGetUniformLocation(programObject_, name.data());
	}

	GLuint getUniformBlockIndex(std::string_view name) const {
		return glGetUniformBlockIndex(programObject_, name.data());
	}

private:
	GLuint programObject_ = 0; // Le ID de programme nuanceur.
	std::unordered_map<GLenum, std::unordered_set<GLuint>> shadersByType_; // Les nuanceurs.
};

// Une variable uniforme qui se rappelle de ses localisations pour chaque programme nuanceur. On peut accéder à la valeur sous-jacente avec get() ou comme un pointeur avec * et ->.
template <typename T>
class Uniform
{
public:
	using value_type = T;

	Uniform(const std::string& name = "", const T& value = {}) {
		reset(name, value);
	}

	Uniform& operator= (const Uniform& other) {
		reset(other.name_, other.value_);
		return *this;
	}

	Uniform& operator= (const T& value) {
		value_ = value;
		return *this;
	}

	// Accès à la valeur.
	const T& get() const { return value_; }
	T& get() { return value_; }
	const T* operator->() const { return &get(); }
	T* operator->() { return &get(); }
	const T& operator*() const { return get(); }
	T& operator*() { return get(); }
	operator T&() const { return get(); }
	operator T&() { return get(); }

	const std::string& getName() const { return name_; }

	void setName(const std::string& name) {
		name_ = name;
		// Changer le nom de la variable invalide les localisations.
		auto oldLocs = std::move(locs_);
		locs_.clear();
		for (auto&& [progObj, loc] : oldLocs)
			getLoc(ShaderProgram(progObj));
	}

	void reset(const std::string& name, const T& value = {}) {
		setName(name);
		value_ = value;
	}

	GLuint getLoc(const ShaderProgram& prog) const {
		auto it = locs_.find(prog.getObject());
		if (it != locs_.end())
			return it->second;
		else
			// Dans la version constante, on ne peut pas modifier les localisations connues, donc on fait la recherche à chaque fois.
			return queryUniformLocation(prog);
	}

	GLuint getLoc(const ShaderProgram& prog) {
		auto it = locs_.find(prog.getObject());
		// Si le programme nuanceur n'est pas reconnu, chercher la localisation et la sauvegarder.
		if (it == locs_.end())
			it = locs_.insert({prog.getObject(), queryUniformLocation(prog)}).first;
		return it->second;
	}

	virtual GLuint queryUniformLocation(const ShaderProgram& prog) const {
		return prog.getUniformLocation(name_);
	}

protected:
	T value_ = {};
	std::string name_;
	std::unordered_map<GLuint, GLuint> locs_;
};

// Un bloc de données uniforme. C'est une variable uniforme mais chargé dans un buffer (un Uniform Buffer Object, ou UBO) et un index plutôt qu'avec des glUniform*. On hérite de Uniform<T> pour réutiliser les fonctionnalités de sauvegarde de localisation.
template <typename T>
class UniformBlock : public Uniform<T>
{
public:
	UniformBlock(const std::string& name = "", GLuint bindingIndex = -1, const T& value = {}) {
		reset(name, bindingIndex, value);
	}

	UniformBlock& operator= (const UniformBlock& other) {
		reset(other.name_, other.bindingIndex_, other.value_);
		return *this;
	}

	UniformBlock& operator= (const T& value) {
		this->get() = value;
		return *this;
	}

	GLuint getUbo() const { return ubo_; }
	GLuint getBindingIndex() const { return bindingIndex_; }

	void reset(const std::string& name, GLuint bindingIndex, const T& value = {}) {
		Uniform<T>::reset(name, value);
		bindingIndex_ = bindingIndex;
	}

	void setup(GLenum usageMode = GL_DYNAMIC_COPY) {
		if (ubo_ == 0)
			glGenBuffers(1, &ubo_);
		glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(this->get()), &this->get(), usageMode);
		glBindBufferBase(GL_UNIFORM_BUFFER, bindingIndex_, ubo_);
	}

	void updateBuffer() {
		glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(this->get()), &this->get());
	}

	void bindToProgram(ShaderProgram& prog) {
		prog.use();
		prog.bindUniformBlock(this->getLoc(prog), bindingIndex_);
	}

	GLuint queryUniformLocation(const ShaderProgram& prog) const override {
		return prog.getUniformBlockIndex(this->getName());
	}

	void deleteObject() {
		glDeleteBuffers(1, &ubo_);
	}

private:
	GLuint ubo_ = 0;
	GLuint bindingIndex_ = -1;
};
