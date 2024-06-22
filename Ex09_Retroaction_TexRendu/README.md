# Textures de rendu

On a une scène dans laquelle se trouve une deuxième caméra (l'oeil jaune) qui observe la scène. Le point de vue de cette caméra est affiché sur un écran accroché au même poteau.

<img src="doc/scene.gif"/>

Pour projeter la vue de la caméra secondaire sur la TV, on dessine la scène dans une texture en mémoire plutôt que dans le tampon de couleur de la fenêtre. Cette texture est ensuite appliquée sur le quad de la TV.

Avant la première trame : Générer un framebuffer avec une texture comme tampon de couleur et un tampon de profondeur. On reproduit essentiellement les tampons de fenêtre.

À chaque trame :
1. Lier le framebuffer de la caméra secondaire.
1. Modifier la matrice de visualisation pour positionner la caméra synthétique juste devant l'oeil jaune.
1. Appliquer une perpective pour la caméra secondaire selon les dimensions de la texture de rendu (pas de la fenêtre).
1. Dessiner la scène. Ça met à jour la texture de rendu directement en mémoire graphique.
1. Délier le framebuffer (donc dessin dans le tampon de fenêtre usuel).
1. Restaurer la caméra orbitale habituelle et la perspective proportionnelle aux dimensions de fenêtre.
1. Dessiner la scène. La texture de rendu est appliquée au quad à l'intérieur de la TV.

## Contrôles

* R : réinitialiser la position de la caméra.
* \+ et - :  rapprocher et éloigner la caméra orbitale.
* haut/bas : changer la latitude de la caméra orbitale.
* gauche/droite : changer la longitude ou le roulement (avec shift) de la caméra orbitale.
* clic central (cliquer la roulette) : bouger la caméra en glissant la souris.
* roulette : rapprocher et éloigner la caméra orbitale.
* espace : mettre en pause le scan de la caméra de surveillance.

