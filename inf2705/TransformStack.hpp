#pragma once


#include <cstddef>
#include <cstdint>

#include <stack>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glbinding/gl/gl.h>


using namespace gl;
using namespace glm;


struct ProjectionBox
{
	float leftFace;
	float rightFace;
	float bottomFace;
	float topFace;
	float nearDist;
	float farDist;
};

// Une pile de matrices de transformations (hérite de `std::stack`). Les tranformations (rotation, translation, etc.) s'opère sur le dessus de la pile. C'est un peu comme la classe `MatricePipeline` des notes de cours. On peut aussi la convertir implicitement en mat4 (ça prend le dessus de la pile) et faire des multiplication directement avec * et *=.
// Les objets de cette classe seront souvent passées à des nuanceurs. Un TransformStack possède un nom correspondant à la variable uniforme qu'il représente. setName() et getName() manipule le nom et getLoc() permet d'obtenir la « localisation » de cette variable uniforme pour un programme OpenGL donné. La recherche de cet objet est fait une fois par programme et celui-ci est conservé par la suite pour éviter les appels répétés à glGetUniformLocation.
class TransformStack : public std::stack<mat4>
{
public:
	using stack<mat4>::stack;

	TransformStack(const std::string& name = "") {
		pushIdentity();
		setName(name);
	}

	TransformStack(const mat4& m) {
		push(m);
	}

	TransformStack& operator= (const mat4& m) {
		top() = m;
		return *this;
	}

	void loadIdentity() {
		top() = mat4(1.0f);
	}
	void scale(const vec3& v) {
		top() = glm::scale(top(), v);
	}
	void translate(const vec3& v) {
		top() = glm::translate(top(), v);
	}
	void rotate(float angleDegrees, const vec3& v) {
		top() = glm::rotate(top(), radians(angleDegrees), v);
	}
	void invert() {
		top() = glm::inverse(top());
	}

	void lookAt(const vec3& eye, const vec3& center, const vec3& up) {
		top() = glm::lookAt(eye, center, up);
	}
	void frustum(const ProjectionBox& plane) {
		top() = glm::frustum(plane.leftFace, plane.rightFace, plane.bottomFace, plane.topFace, plane.nearDist, plane.farDist);
	}
	void frustum(float leftFace, float rightFace, float bottomFace, float topFace, float nearDist, float farDist) {
		frustum({leftFace, rightFace, bottomFace, topFace, nearDist, farDist});
	}
	void perspective(float fovyDegrees, float aspect, float nearDist, float farDist) {
		top() = glm::perspective(radians(fovyDegrees), aspect, nearDist, farDist);
	}
	void ortho(const ProjectionBox& plane) {
		top() = glm::ortho(plane.leftFace, plane.rightFace, plane.bottomFace, plane.topFace, plane.nearDist, plane.farDist);
	}
	void ortho(float leftFace, float rightFace, float bottomFace, float topFace, float nearDist, float farDist) {
		ortho({leftFace, rightFace, bottomFace, topFace, nearDist, farDist});
	}
	void ortho2D(const ProjectionBox& plane) {
		top() = glm::ortho(plane.leftFace, plane.rightFace, plane.bottomFace, plane.topFace);
	}
	void ortho2D(float leftFace, float rightFace, float bottomFace, float topFace) {
		ortho2D({leftFace, rightFace, bottomFace, topFace});
	}

	TransformStack& operator*= (const mat4& matrix) {
		top() *= matrix;
		return *this;
	}

	TransformStack& operator*= (const TransformStack& other) {
		*this *= other.top();
		return *this;
	}

	mat4 operator* (const mat4& matrix) const {
		return top() * matrix;
	}

	vec4 operator* (const vec4& vect) const {
		return top() * vect;
	}

	vec4 operator* (const vec3& vect) const {
		return top() * vec4(vect, 1.0f);
	}

	operator mat4() const {
		return top();
	}

	using stack<mat4>::push;
	using stack<mat4>::pop;

	void push() {
		if (empty())
			pushIdentity();
		else
			push(top());
	}

	void pushIdentity() {
		push(mat4(1.0f));
	}

	const std::string& getName() const { return name_; }

	void setName(const std::string& name) {
		// Changer le nom de la variable invalide les localisations.
		name_ = name;
		auto oldLocs = std::move(locs_);
		locs_.clear();
		for (auto&& [progObj, loc] : oldLocs)
			getLoc(progObj);
	}

	// Obtenir la localisation pour un programme donné par son objet (son identifiant).
	GLuint getLoc(GLuint prog) const {
		auto it = locs_.find(prog);
		if (it != locs_.end())
			return it->second;
		else
			// Dans la version constante, on ne peut pas modifier les localisations connues, donc on fait la recherche à chaque fois.
			return glGetUniformLocation(prog, name_.c_str());
	}

	// Obtenir la localisation pour un programme donné par son objet (son identifiant).
	GLuint getLoc(GLuint prog) {
		auto it = locs_.find(prog);
		// Si le programme nuanceur n'est pas reconnu, chercher la localisation et la sauvegarder.
		if (it == locs_.end()) {
			GLuint loc = glGetUniformLocation(prog, name_.c_str());
			it = locs_.insert({prog, loc}).first;
		}
		return it->second;
	}

private:
	std::string name_;
	std::unordered_map<GLuint, GLuint> locs_;
};

