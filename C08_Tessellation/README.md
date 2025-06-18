# Nuanceurs de tessellation

## Forme de base et subdivision

On a à la base un icosaèdre (*d20*) régulier, c'est-à-dire dont tous les sommets ont une distance de 1 par rapport à l'origine.

<img src="doc/d20.png"/>

Les nuanceurs de tessellation le transforment en sphère en subdivisant ses faces et en repositionnant les sommets créés.

La forme (en fil de fer) générée par 4 niveaux de tessellation :

<img src="doc/d20_tess_wire.png"/>

La même forme texturée par rapport au mesh de base :

<img src="doc/d20_tess_solid.png"/>

## Repositionnement des sommets

Dans le nuanceur d'évaluation de la tessellation, n veut créer une sphère, mais on ne connait pas son rayon. Assumer rayon=1 est une mauvaise idée. On pourrait le mettre en variable uniforme, mais c'est mieux de tout simplement calculer le rayon local à la primitive courante (celle qui génère le sommet tessellé). Ça permet aussi de garder une géométrie sur laquelle on peut par la suite appliquer une matrice de modélisation (avec étirement, déplacement, etc.).

Dans [sphere_tesseval.glsl](sphere_tesseval.glsl), on fait la moyenne pondérée (avec les `gl_TessCoord`) pour trouver la position moyenne du nouveau sommet. On normalise ensuite ce vecteur, ce qui nous donne la direction du nouveau sommet par rapport à l'origine. On multiplie ce vecteur par la moyenne pondérée des rayons des trois sommets de la primitive et ça le repositionne sur la surface sphérique locale.

<img src="doc/avg.png"/>

## Contrôles

* F5 : capture d'écran.
* R : réinitialiser la position de la caméra.
* \+ et - :  rapprocher et éloigner la caméra orbitale.
* haut/bas : changer la latitude de la caméra orbitale.
* gauche/droite : changer la longitude ou le roulement (avec shift) de la caméra orbitale.
* clic droit ou central : bouger la caméra en glissant la souris.
* roulette : rapprocher et éloigner la caméra orbitale.
* 1 : afficher en faces pleines ou wireframe.
* 2 : montrer ou cacher les arêtes de la forme originale (avant tessellation).
* 3 : activer/désactiver le cull des faces arrières.
* W et S : étirer/compresser le d20 en Y.
* A et D : étirer/compresser le d20 en XZ.
* I et shift+I : augmenter/diminuer le niveau de tessellation intérieur.
* O et shift+O : augmenter/diminuer le niveau de tessellation extérieur.