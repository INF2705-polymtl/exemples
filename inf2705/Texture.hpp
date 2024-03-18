#pragma once


#include <cstddef>
#include <cstdint>

#include <string>
#include <format>

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <SFML/Graphics.hpp>

#include "sfml_utils.hpp"
#include "ShaderProgram.hpp"


using namespace gl;
using namespace glm;


struct Texture
{
	GLuint id = 0;
	ivec2 size = {};
	int numLevels = 0;

	void bindToTextureUnit(int textureUnit) {
		glActiveTexture(GL_TEXTURE0 + textureUnit);
		glBindTexture(GL_TEXTURE_2D, id);
	}

	void bindToTextureUnit(int textureUnit, ShaderProgram& prog, std::string_view name) {
		bindToTextureUnit(textureUnit, prog, prog.getUniformLocation(name));
	}

	void bindToTextureUnit(int textureUnit, ShaderProgram& prog, GLuint loc) {
		bindToTextureUnit(textureUnit);
		prog.use();
		prog.setTextureUnit(loc, textureUnit);
	}

	void setPixelData(GLenum format, const void* data) {
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			format,
			size.x,
			size.y,
			0,
			format,
			GL_UNSIGNED_BYTE,
			data
		);
	}

	// Si detailLevels est > 1, demande à OpenGL de générer les mipmaps.
	static Texture loadFromImage(const sf::Image& img, int detailLevels = 1) {
		// Beaucoup de bibliothèques importent les images avec x=0,y=0 (donc premier pixel du tableau) au coin haut-gauche de l'image. C'est la convention en graphisme, mais les textures en OpenGL ont leur origine au coin bas-gauche.
		// SFML applique la convention origine = haut-gauche, il faut donc renverser l'image verticalement avant de la passer à OpenGL.
		sf::Image texImg = img;
		texImg.flipVertically();

		// Générer et lier un objet de texture. Ça ressemble un peu aux VBO.
		Texture tex = {};
		tex.size = {texImg.getSize().x, texImg.getSize().y};
		tex.numLevels = detailLevels;
		glGenTextures(1, &tex.id);
		glBindTexture(GL_TEXTURE_2D, tex.id);
		// Passer les données de l'image (un peu comme avec glBufferData). Il faut spécifier le format interne qui sera enregistré sur le GPU ainsi que celui dont est fait le tableau de données passé en paramètre.
		tex.setPixelData(GL_RGBA, texImg.getPixelsPtr());

		// Le paramètre contrôle la génération automatique de mipmaps.
		if (detailLevels > 1) {
			// Spécifier le mode de filtrage pour la minimisation (zoom out). Si on utilise du mipmap, il faut utiliser le mode spécifique aux mipmaps (GL_NEAREST_MIPMAP_* ou GL_LINEAR_MIPMAP_*)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			// Spécifier le mode de filtrage pour le grossissement (zoom in). Beaucoup de ressources en ligne font l'erreur d'utiliser GL_*_MIPMAP_* pour le grossissement. Les seules valeurs applicables sont GL_LINEAR et GL_NEAREST.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// Générer automatiquement les mipmaps. L'algorithme utilisé pour faire la mise à l'échelle n'est pas spécifiée dans le standard OpenGL. C'est un compromis entre la solution simple (pas de mipmap) et la solution compliqué (mipmap manuel).
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, detailLevels - 1);
			glGenerateMipmap(GL_TEXTURE_2D);
		} else {
			// Spéficier les modes de filtrage. GL_NEAREST pour la minimisation et GL_LINEAR pour le grossissement fonctionnent bien dans une majorité des cas.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		return tex;
	}

	// Si detailLevels est > 1, demande à OpenGL de générer les mipmaps.
	static Texture loadFromFile(const std::string& filename, int detailLevels = 1) {
		// Lire les pixels de l'image. SFML (la bibliothèque qu'on utilise pour gérer la fenêtre) a déjà une fonctionnalité de chargement d'images. Une alternative plus légère est stb_image.
		sf::Image texImg;
		if (not texImg.loadFromFile(filename)) {
			std::cerr << std::format("{} could not be loaded", filename) << "\n";
			return {};
		}
		return loadFromImage(texImg, detailLevels);
	}

	// filenamePattern doit contenir un "{}" qui sera remplacé par 0 à numLevels (avec un format de spécification optionnel comme en Python).
	static Texture loadFromMipmapFiles(const std::string& filenamePattern, int numLevels) {
		// Créer et lier l'objet de texture. Quand on fait des mipmap manuellement, il faut créer une seule texture à laquelle on passe une image différente pour chaque niveau de détail.
		Texture result = {};
		glGenTextures(1, &result.id);
		glBindTexture(GL_TEXTURE_2D, result.id);
		// Pour chaque niveau de détails:
		for (int i = 0; i < numLevels; i++) {
			// Générer le nom de fichier (du beau C++20).
			auto filename = std::vformat(filenamePattern, std::make_format_args(i));
			// Charger l'image et la renverser verticalement (voir commentaire dans fonction précédente).
			sf::Image texImg;
			if (not texImg.loadFromFile(filename))
				throw std::runtime_error(std::format("{} could not be loaded", filename));
			texImg.flipVertically();
			// Passer l'image en spécifiant le niveau de détail (2e paramètre de glTexImage2D).
			glTexImage2D(
				GL_TEXTURE_2D,
				i,
				GL_RGBA,
				texImg.getSize().x,
				texImg.getSize().y,
				0,
				GL_RGBA,
				GL_UNSIGNED_BYTE,
				texImg.getPixelsPtr()
			);
			if (i == 0)
				result.size = {texImg.getSize().x, texImg.getSize().y};
		}

		// Spécifier le mode de filtrage. On utilise les mêmes que si on utilisait glGenerateMipmap.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Spécifier (optionnellement) le niveau de détail correspondant à la définition de base. Par défaut c'est 0 et donc pas nécessaire de le modifier.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		// ATTENTION: Ce n'est pas super clair dans la documentation officielle, mais il faut configurer le GL_TEXTURE_MAX_LEVEL quand on fait des mipmap manuellement. Le défaut est 1000, il s'attend donc à recevoir 1000 tableaux de pixels.
		// Dans notre cas on a 6 niveaux, donc 0 à 5, alors on passe 5.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numLevels - 1);
		result.numLevels = numLevels;

		return result;
	}
};

// Une texture liée à une variable uniforme et une unité active.
struct BoundTexture
{
	Texture* texture; // La texture référencée
	Uniform<int> activeUnit; // L'unité active (les GL_TEXTURE*) qui est la variable uniforme à mettre à jour

	GLuint getLoc(const ShaderProgram& prog) {
		return activeUnit.getLoc(prog);
	}

	void bindToProgram(ShaderProgram& prog) {
		texture->bindToTextureUnit(activeUnit, prog, getLoc(prog));
	}
};

