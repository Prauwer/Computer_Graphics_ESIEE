# Projet Computer Graphics (4I_IG1) par Antonin Mansour - Zackary Saada - Jovan Rahme

Ce projet est une application OpenGL avancée explorant diverses techniques de rendu 3D, de l'initialisation simple aux effets de post-traitement complexes et à l'intégration d'une interface utilisateur interactive. Il sert de bac à sable pour comprendre et implémenter les concepts fondamentaux de la synthèse d'images en temps réel.

![image](https://github.com/user-attachments/assets/60611986-53ea-4abd-b5a2-2bb25be0d428)

## ✨ Fonctionnalités Clés

Ce projet met en œuvre les fonctionnalités suivantes, structurées en modules logiques :

### 1. **Fondations du Moteur 3D**

* **Initialisation du Projet & Gestion des Shaders :** Mise en place d'un pipeline de rendu OpenGL avec GLFW pour la gestion de la fenêtre et GLEW pour les extensions. Implémentation d'une classe `GLShader` robuste pour la compilation et la liaison des shaders GLSL, permettant une gestion simplifiée des programmes.

* **Mathématiques 3D Essentielles (`Mat4`) :** Intégration d'une classe `mat4` pour toutes les transformations matricielles (modèle, vue, projection), optimisée pour les opérations 3D.

* **Chargement et Rendu de Modèles OBJ :** Capacité à charger des modèles 3D au format `.OBJ` grâce à `tiny_obj_loader`. Le projet gère la triangulation des maillages, les normales et les coordonnées UV, permettant un rendu basique de géométries complexes.

### 2. **Manipulation des Objets et de la Scène**

* **Transformations d'Objets (Translation, Rotation, Scale) :** Chaque objet de la scène peut être positionné, orienté et redimensionné indépendamment en utilisant des matrices de modèle. Cela permet une composition dynamique de la scène.

* **Caméra Mobile et Orbitale :** Le projet inclut un système de caméra flexible :

  * **Caméra Orbitale :** Contrôle la distance, le lacet (yaw) et le tangage (pitch) autour d'un point d'intérêt, offrant une vue exploratoire de la scène. Les interactions se font via la souris (clic-glisser pour orbiter, molette pour le zoom).

### 3. **Rendu Visuel et Éclairage**

* **Shaders Avancés et Textures :**

  * **Shader Texturé :** Application de textures 2D sur des objets, avec gestion du chargement d'images via `stb_image`. Le projet démontre cela avec un deuxième objet (une pomme).

  * **Environment Mapping (Env Map) :** Implémentation d'une technique d'éclairage basée sur l'environnement, où un troisième objet (une sphère) reflète la scène environnante via une cubemap, simulant des matériaux réfléchissants.

  * **Skybox :** Intégration d'une boîte céleste qui enveloppe la scène, offrant un arrière-plan immersif et un éclairage d'environnement réaliste pour les objets réfléchissants.

* **Éclairage Phong/Blinn-Phong :** Mise en œuvre du modèle d'éclairage de Phong (ou Blinn-Phong), simulant l'interaction de la lumière avec les matériaux de l'objet (diffuse, spéculaire, ambiante). Le cube de test utilise cet éclairage pour des rendus plus réalistes.

* **Gestion du sRGB :** Prise en charge du profil de couleur sRGB (`GL_FRAMEBUFFER_SRGB`) pour garantir une correction gamma précise et un affichage des couleurs fidèles à l'intention artistique.

### 4. **Optimisations et Effets Post-Traitement**

* **Uniform Buffer Objects (UBO) :** Utilisation des UBOs pour regrouper et envoyer efficacement les données uniformes (comme les matrices de vue et de projection) aux shaders. Cela optimise les performances en réduisant le nombre d'appels `glUniform`.

* **Framebuffer Objects (FBO) et Post-traitement :** Le rendu de la scène est d'abord effectué dans un FBO (Framebuffer Object) au lieu du framebuffer par défaut. Le contenu du FBO est ensuite appliqué sur un quad plein écran, permettant d'appliquer divers effets de post-traitement en temps réel.

  * **Effets Disponibles :** Niveaux de gris, inversion des couleurs et sépia.

  * **Contrôles Colorimétriques :** Réglages de saturation et de contraste via l'interface utilisateur.

* **Intégration ImGui (Interface Utilisateur) :** ImGui est intégré pour fournir une interface utilisateur graphique interactive. Cela permet aux utilisateurs de contrôler dynamiquement les paramètres de post-traitement (choix de l'effet, saturation, contraste) en temps réel, offrant une expérience exploratoire riche. Les interactions de la souris et du clavier sont redirigées intelligemment vers ImGui ou vers les contrôles de la caméra selon le besoin.

## 🛠️ Compilation et Exécution

Pour compiler et exécuter ce projet, assurez-vous d'avoir les dépendances suivantes installées :

* **GLFW :** Pour la gestion de la fenêtre, des entrées et du contexte OpenGL.

* **GLEW (Windows/Linux) ou OpenGL Framework (macOS) :** Pour gérer les extensions OpenGL.

* **TinyOBJLoader :** Inclus en tant que source unique dans le projet.

* **stb_image :** Inclus en tant que source unique pour le chargement des images.

* **ImGui :** Inclus en tant que source, avec les backends pour GLFW et OpenGL.

**Structure du Projet :**

```
.
├── .gitignore
├── GLShader.cpp
├── GLShader.h
├── main.cpp
├── mat4.h
├── Makefile
├── assets/
│   ├── 3DApple002_SQ-1K-PNG/
│   │   ├── 3DApple002_SQ-1K-PNG.obj
│   │   └── 3DApple002_SQ-1K-PNG_Color.png
│   ├── cloudy/
│   └── Yokohama3/
│   ├── cube.obj
│   ├── sphere.obj
├── libs/
│   ├── imgui/
│   │   ├── imconfig.h
│   │   ├── imgui.cpp
│   │   ├── imgui.h
│   │   ├── imgui_draw.cpp
│   │   ├── imgui_impl_glfw.cpp
│   │   ├── imgui_impl_glfw.h
│   │   ├── imgui_impl_opengl3.cpp
│   │   ├── imgui_impl_opengl3.h
│   │   ├── imgui_impl_opengl3_loader.h
│   │   ├── imgui_internal.h
│   │   ├── imgui_tables.cpp
│   │   ├── imgui_widgets.cpp
│   │   ├── imstb_rectpack.h
│   │   ├── imstb_textedit.h
│   │   └── imstb_truetype.h
│   ├── stb/
│   │   └── stb_image.h
│   └── tiny_obj_loader.h
└── shaders/
    ├── basic.fs
    ├── basic.vs
    ├── env.fs
    ├── env.vs
    ├── phong.fs
    ├── phong.vs
    ├── screen_quad.fs
    ├── screen_quad.vs
    ├── skybox.fs
    ├── skybox.vs
    ├── texture.fs
    └── texture.vs
```
**Compilation et Exécution :**

Pour compiler le projet, naviguez dans le répertoire racine du projet et exécutez la commande `make` :
```
make
```

Une fois la compilation réussie, vous pouvez exécuter l'application :
```
./ESIEE_Computer_Graphics
```

## 🎮 Utilisation

* **Contrôle de la Caméra :**

  * **Clic gauche + Glisser :** Orbite autour du centre de la scène.

  * **Molette de la souris :** Zoom avant/arrière.

* **Interface ImGui :**

  * Le panneau "Options de Post-traitement" en haut à gauche vous permet de :

    * Choisir parmi différents effets de post-traitement (Aucun, Niveaux de gris, Inverser couleurs, Sépia).

    * Ajuster la saturation et le contraste de la scène en temps réel.
