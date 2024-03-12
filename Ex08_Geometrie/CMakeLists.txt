cmake_minimum_required(VERSION 3.0.0)
# La raison pour laquelle on fait une variable d'environnement VCPKG_ROOT.
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

# Le nom du projet.
project(INF2705_Ex08_Geometrie)

# On met les fichiers sources (incluant les entêtes)
set(ALL_FILES
    "main.cpp"
    "../inf2705/Mesh.hpp"
    "../inf2705/OpenGLApplication.hpp"
    "../inf2705/OrbitCamera.hpp"
    "../inf2705/ShaderProgram.hpp"
    "../inf2705/SceneObject.hpp"
    "../inf2705/sfml_utils.hpp"
    "../inf2705/Texture.hpp"
    "../inf2705/TransformStack.hpp"
    "../inf2705/utils.hpp"
)
add_executable(${PROJECT_NAME} ${ALL_FILES})

include_directories("../")

# Les flags de compilation.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fsigned-char -Wno-unknown-pragmas -D WIN32_LEAN_AND_MEAN -D NOMINMAX -D _CRT_SECURE_NO_WARNINGS -D GLM_FORCE_SWIZZLE")

# Tous ce qui suit sont des package Vcpkg. Pour savoir quoi mettre, on fait "vcpkg install <le package>", puis on met ce qu'ils disent de mettre dans le CMakeLists.
# Vous pouvez commenter/décommenter les packages que vous utilisez ou pas.

# GLM: Pour les math comme en GLSL.
find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)

# SFML: Pour la gestion de fenêtre et d'événements.
#       Tout en C++, assez clean et léger.
find_package(SFML COMPONENTS system window graphics CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-system sfml-network sfml-graphics sfml-window)

# glbinding: Pour l'importation des fonctions OpenGL et la résolution d'adresses.
#            Tout en C++ assez moderne, très clean avec des enum, des namespace et peu de macros.
find_package(glbinding CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glbinding::glbinding glbinding::glbinding-aux)

# tinyobjloader: Pour l'importation des mesh à partir de fichiers Wavefront.
find_package(tinyobjloader CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE tinyobjloader::tinyobjloader)
