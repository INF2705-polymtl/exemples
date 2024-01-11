# Exemples INF2705 (par chosson)

Les exemples du cours INF2705 - Infographie tels qu'ils sont présentés en classe par l'enseignant Charles Hosson (*chosson*). Ils ne sont pas aussi collés sur les notes de cours originales que les [exemples de Benoît Ozell](https://gitlab.com/ozell/inf2705-exemples) (l'ancien coordonnateur et éternel gourou spirituel du cours d'infographie).

## Structure du code

La majorité des exemples utilisent les bibliothèques [glbinding](https://glbinding.org) pour l'importation des fonctions et valeurs d'OpenGL et [SFML](https://www.sfml-dev.org) pour la gestion de fenêtre et d'événements. D'autres bibliothèques sont aussi utilisées pour le chargement d'image, de son, etc.

Chaque exemple (généralement un par séance de cours) est dans un dossier Ex*NN*_*Nom* qui contient le code source et tous les fichiers nécessaires à l'exemple. Dans le dossier [inf2705](inf2705) se trouvent les entêtes du code qui est commun à tous les exemples, tels qu'une classe de base pour les applications. On pourrait dire que c'est le *cadriciel* pour les exemples du cours.

Le code source des exemples fait généralement les inclusions des entêtes communes comme suit :

```c++
#include <inf2705/OpenGLApplication.hpp>
```

C'est plus propre que `#include "../inf2705/BlaBla.hpp"` mais ça implique d'ajouter le dossier racine comme dossiers de *include* dans les options de compilateur.

## Compilation

Les exemples ont chacun un projet Visual Studio (tel que [Ex02_Pipeline.vcxproj](Ex02_Pipeline/Ex02_Pipeline.vcxproj)) qui sont tous organisés dans une solution VS ([INF2705_Exemples.sln](INF2705_Exemples.sln)) pour compiler sur Windows. Les projets VS n'ont pas de configuration spéciale pour lier les bibliothèques, ils fonctionnent de façon transparentes si les bibliothèques ont été installées avec Vcpkg. Ils ont aussi l'ajout du dossier principal pour l'inclusion des entêtes communes.

