
#ifdef _WIN32
#include <GL/glew.h>
#include <GL/wglew.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <OpenGL/OpenGL.h>
#endif

#include <GLFW/glfw3.h>

#include <cstddef>
#include <stdio.h>
#include <cmath>
#include "mat4.h" // Added mat4.h
#include "GLShader.h"
#include <iostream>
#include <vector>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h" // Adjusted path assuming it's directly in libs/

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

// Global variables
GLShader g_BasicShader;
GLShader g_TextureShader;
GLShader  g_EnvShader;
GLShader g_SkyboxShader;
GLFWwindow* g_window;

GLuint g_mainTex = 0; // Global texture object
GLuint secondTex = 0; // Global texture object
GLuint envCubemap = 0;
GLuint  skyboxVAO = 0, skyboxVBO = 0;

// Variables pour la caméra orbitale
float g_cameraDistance = 5.0f;
float g_cameraYaw = 0.0f;      // Angle horizontal autour de l'axe Y
float g_cameraPitch = 0.0f;    // Angle vertical par rapport au plan XZ

bool g_isDragging = false;
double g_lastMouseX = 0.0;
double g_lastMouseY = 0.0;

// Callback functions for GLFW
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// Structure to hold 3D model data (VAO, VBO, etc.)
struct Model {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    int indexCount = 0;
};

Model g_mainModel; // Global model object
Model g_secondModel; // Global model object
Model g_envModel;


// Structure pour un sommet 3D, incluant position, normale et coordonnées de texture (UV).
struct Vertex {
    float position[3]; // x, y, z
    float normal[3]; // nx, ny, ny
    float uv[2]; // u, v

    bool operator==(const Vertex& other) const {
        return memcmp(this, &other, sizeof(Vertex)) == 0;
    }
};

// Spécialisation du hasher pour la structure Vertex, nécessaire pour l'utiliser dans un std::unordered_map.
// Cela permet d'optimiser le chargement de l'OBJ en évitant les sommets dupliqués.
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            // Simple hash combination logic
            size_t seed = 0;
            hash<float> hasher;
            seed ^= hasher(vertex.position[0]) + 0x9e3779b9 + (seed<<6) + (seed>>2);
            seed ^= hasher(vertex.position[1]) + 0x9e3779b9 + (seed<<6) + (seed>>2);
            seed ^= hasher(vertex.position[2]) + 0x9e3779b9 + (seed<<6) + (seed>>2);
            seed ^= hasher(vertex.normal[0]) + 0x9e3779b9 + (seed<<6) + (seed>>2);
            seed ^= hasher(vertex.normal[1]) + 0x9e3779b9 + (seed<<6) + (seed>>2);
            seed ^= hasher(vertex.normal[2]) + 0x9e3779b9 + (seed<<6) + (seed>>2);
            seed ^= hasher(vertex.uv[0]) + 0x9e3779b9 + (seed<<6) + (seed>>2);
            seed ^= hasher(vertex.uv[1]) + 0x9e3779b9 + (seed<<6) + (seed>>2);
            return seed;
        }
    };
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    // glfwSetWindowSize(window, 640, 480);
}

GLuint loadTexture(const char* path) {
    int w, h, n;
    unsigned char* data = stbi_load(path, &w, &h, &n, STBI_rgb_alpha);
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return tex;
}

void layout() {
    // Position (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    // Normale (3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    // UV (2 floats)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
}


// New function to load an OBJ file and create a Model
Model loadObjModel(const std::string& filepath) {
    Model model;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::unordered_map<Vertex, unsigned int> uniqueVertices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};

            vertex.position[0] = attrib.vertices[3 * index.vertex_index + 0];
            vertex.position[1] = attrib.vertices[3 * index.vertex_index + 1];
            vertex.position[2] = attrib.vertices[3 * index.vertex_index + 2];

            if (index.normal_index >= 0) {
                vertex.normal[0] = attrib.normals[3 * index.normal_index + 0];
                vertex.normal[1] = attrib.normals[3 * index.normal_index + 1];
                vertex.normal[2] = attrib.normals[3 * index.normal_index + 2];
            }

            if (index.texcoord_index >= 0) {
                vertex.uv[0] = attrib.texcoords[2 * index.texcoord_index + 0];
                vertex.uv[1] = attrib.texcoords[2 * index.texcoord_index + 1];
            }

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<unsigned int>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
        }
    }

    model.indexCount = indices.size();

    glGenVertexArrays(1, &model.vao);
    glBindVertexArray(model.vao);

    glGenBuffers(1, &model.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &model.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    layout();

    glBindVertexArray(0);

    return model;
}


GLuint loadCubemap(const std::vector<std::string>& faces)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    int w, h, n;
    for (GLuint i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &n, STBI_rgb_alpha);
        if (data) {
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        } else {
            std::cerr << "Cubemap load failed: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    // Paramètres
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,     GL_CLAMP_TO_EDGE);

    return texID;
}


bool Initialise()
{
    // --- 1ᵉ shader : Cube SIMPLE
    g_BasicShader.LoadVertexShader("shaders/basic.vs");
    g_BasicShader.LoadFragmentShader("shaders/basic.fs");
    g_BasicShader.Create();
    
    // --- 2ᵉ shader : POMME Texturée
    g_TextureShader.LoadVertexShader("shaders/texture.vs");
    g_TextureShader.LoadFragmentShader("shaders/texture.fs");
    g_TextureShader.Create(); 

    // --- 3ᵉ shader : envmap
    g_EnvShader.LoadVertexShader("shaders/env.vs");
    g_EnvShader.LoadFragmentShader("shaders/env.fs");
    g_EnvShader.Create();

    // —– 4ᵉ shader : skybox
    g_SkyboxShader.LoadVertexShader  ("shaders/skybox.vs");
    g_SkyboxShader.LoadFragmentShader("shaders/skybox.fs");
    g_SkyboxShader.Create();

    // —– Création du VAO/VBO skybox (cube unit)
    float skyboxVerts[] = {
        // 36 sommets (6 faces × 2 triangles × 3 coords)
        -1,  1, -1,  -1, -1, -1,   1, -1, -1,
         1, -1, -1,   1,  1, -1,  -1,  1, -1,
        // … répétez pour les 5 autres faces : +Z, -X, +X, +Y, -Y (cf. LearnOpenGL)
        // vous trouverez la liste complète dans le TP PDF ou sur LearnOpenGL
    };
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVerts), skyboxVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glBindVertexArray(0);

    #ifdef WIN32
    wglSwapIntervalEXT(1);
    #endif

    secondTex = loadTexture("assets/3DApple002_SQ-1K-PNG/3DApple002_SQ-1K-PNG_Color.png"); 

    try {
        g_mainModel = loadObjModel("assets/cube.obj"); // modele .obj du cub
        g_secondModel = loadObjModel("assets/3DApple002_SQ-1K-PNG/3DApple002_SQ-1K-PNG.obj"); // model .obj de la pomme

        // --- chargement du modèle sphère
        g_envModel = loadObjModel("assets/sphere.obj");

    } catch (const std::exception& e) {
        std::cerr << "Error loading model: " << e.what() << std::endl;
        return false;
    }

    // --- chargement de la cubemap
    envCubemap = loadCubemap({
        "assets/cloudy/bluecloud_rt.jpg",
        "assets/cloudy/bluecloud_lf.jpg",
        "assets/cloudy/bluecloud_up.jpg",
        "assets/cloudy/bluecloud_dn.jpg",
        "assets/cloudy/bluecloud_ft.jpg",
        "assets/cloudy/bluecloud_bk.jpg"
    });

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    return true;
}



void Render()
{
    // Étape a. Récupération des dimensions du framebuffer
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    glViewport(0, 0, width, height);

    // Étape b. Effacement du framebuffer
    glClearColor(0.75f, 0.75f, 0.75f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // ────────────────────────────────────────────────────────────────
    // Avant tout skybox ou objets, calculons la VIEW et la PROJ :
    // ────────────────────────────────────────────────────────────────

    float aspectRatio = (height > 0) ? ((float)width / height) : 1.0f;

    // Calcul de la caméra sphérique
    float camX = g_cameraDistance * sin(g_cameraYaw) * cos(g_cameraPitch);
    float camY = g_cameraDistance * sin(g_cameraPitch);
    float camZ = g_cameraDistance * cos(g_cameraYaw) * cos(g_cameraPitch);

    mat4 viewMatrix = mat4::lookAt(
        vec3(camX, camY, camZ),
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 1.0f, 0.0f)
    );

    mat4 projectionMatrix = mat4::perspective(
        45.0f * 3.14159f / 180.0f,
        aspectRatio,
        0.01f,
        100.0f
    );

    // ────────────────────────────────────────────────────────────────
    // 1) DESSIN DU SKYBOX 
    // ────────────────────────────────────────────────────────────────

    // – Désactive l’écriture dans le depth buffer
    glDepthMask(GL_FALSE);

    glUseProgram(g_SkyboxShader.GetProgram());

    // view sans translation : on prend juste la rotation de viewMatrix
    mat4 viewNoTrans = viewMatrix;
    float* p = const_cast<float*>( viewNoTrans.getPtr() );
    p[12] = 0.0f;  // colonne 4, ligne 1
    p[13] = 0.0f;  // colonne 4, ligne 2
    p[14] = 0.0f;  // colonne 4, ligne 3    
    
    GLint loc = glGetUniformLocation(g_SkyboxShader.GetProgram(), "u_view");
    glUniformMatrix4fv(loc,1,GL_FALSE, viewNoTrans.getPtr());
    loc = glGetUniformLocation(g_SkyboxShader.GetProgram(),"u_projection");
    glUniformMatrix4fv(loc,1,GL_FALSE,projectionMatrix.getPtr());

    // lie la même cubemap
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glUniform1i(glGetUniformLocation(g_SkyboxShader.GetProgram(),"u_skybox"), 0);

    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // – Réactive le depth write
    glDepthMask(GL_TRUE);


    // ────────────────────────────────────────────────────────────────
    // 2) DESSIN DU CUBE
    // ────────────────────────────────────────────────────────────────

    // Étape c1. Sélection et utilisation du shader basique pour le cube
    auto basicProgram = g_BasicShader.GetProgram();
    glUseProgram(basicProgram); 

    // Étape d1. Configuration des matrices MVP (identique à votre code)

    // Matrice Modèle (transformations de l'objet)
    // Simplification pour le débogage : on utilise une matrice identité.
    // L'objet sera à l'origine, sans rotation ni scale.
        // On réintroduit une matrice de transformation pour ajuster la taille.
    float rotationXAngle = 20.0f * 3.1415926535f / 180.0f; // 20 degrés en radians

    mat4 modelCube = 
    mat4::translate(-1.0f, 0.5f, 0.0f)    // décale le cube à gauche
  * mat4::rotateX(rotationXAngle)
  * mat4::scale(1.0f, 1.0f, 1.0f);        // échelle réduite pour un cube de taille raisonnable

    // Récupérer les localisations des uniformes
    GLint modelLoc = glGetUniformLocation(basicProgram, "u_model");
    GLint viewLoc = glGetUniformLocation(basicProgram, "u_view");
    GLint projectionLoc = glGetUniformLocation(basicProgram, "u_projection");
    GLint colorLoc = glGetUniformLocation(basicProgram, "u_color");

    // Envoyer les matrices au shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelCube.getPtr());
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix.getPtr());
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projectionMatrix.getPtr());
    glUniform3f(colorLoc, 0.0f, 0.0f, 1.0f);   // cube bleu
    
    // Étape e1. Activation du VAO du cube
    glBindVertexArray(g_mainModel.vao);

    // Étape f1. Dessin du cube
    glDrawElements(GL_TRIANGLES, g_mainModel.indexCount, GL_UNSIGNED_INT, 0);

    // Étape g1. Désactivation du VAO
    glBindVertexArray(0);


    // ────────────────────────────────────────────────────────────────
    // 3) DESSIN DE LA POMME
    // ────────────────────────────────────────────────────────────────

    // Étape c2. Sélection du shader texturé
    auto  secondProgram = g_TextureShader.GetProgram();
    glUseProgram(secondProgram);

    // Étape d2. (On peut réutiliser viewMatrix et projectionMatrix calculées plus haut)
    // Matrice Modèle pour la pomme
    float rotationXAngleApple = 5.0f * 3.1415926535f / 180.0f; // 20 degrés en radians

    mat4 modelApple =
    mat4::translate( 1.0f, 0.0f, 0.0f)    // décale la pomme à droite
  * mat4::rotateX(rotationXAngleApple)
  * mat4::scale(20.0f, 20.0f, 20.0f);     // garde votre facteur d’échelle


    // Récupérer les uniformes (shader texturé)
    GLint modelLocT      = glGetUniformLocation(secondProgram, "u_model");
    GLint viewLocT       = glGetUniformLocation(secondProgram, "u_view");
    GLint projectionLocT = glGetUniformLocation(secondProgram, "u_projection");
    GLint texLoc         = glGetUniformLocation(secondProgram, "u_texture");

    // Envoyer les matrices
    glUniformMatrix4fv(modelLocT,      1, GL_FALSE, modelApple.getPtr());
    glUniformMatrix4fv(viewLocT,       1, GL_FALSE, viewMatrix.getPtr());
    glUniformMatrix4fv(projectionLocT, 1, GL_FALSE, projectionMatrix.getPtr());

    // Étape e2. Application de la texture
    glUniform1i(texLoc, 0);                    // sampler2D sur unité 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, secondTex);

    // Étape f2. Dessin du modèle
    glBindVertexArray(g_secondModel.vao);
    glDrawElements(GL_TRIANGLES, g_secondModel.indexCount, GL_UNSIGNED_INT, 0);

    // Étape g2. Désactivation du VAO
    glBindVertexArray(0);

    // ────────────────────────────────────────────────────────────────
    // 4) DESSIN DE LA SPHÈRE ENVMAP
    // ────────────────────────────────────────────────────────────────

    glUseProgram(g_EnvShader.GetProgram());

    // → on réutilise viewMatrix & projectionMatrix calculées en “d1/d2”
    mat4 modelEnv = 
        mat4::translate(0.0f, 1.0f, 0.0f)
      * mat4::scale     (1.5f, 1.5f, 1.5f);

    loc = glGetUniformLocation(g_EnvShader.GetProgram(), "u_model");
    glUniformMatrix4fv(loc, 1, GL_FALSE, modelEnv.getPtr());
    loc = glGetUniformLocation(g_EnvShader.GetProgram(), "u_view");
    glUniformMatrix4fv(loc, 1, GL_FALSE, viewMatrix.getPtr());
    loc = glGetUniformLocation(g_EnvShader.GetProgram(), "u_projection");
    glUniformMatrix4fv(loc, 1, GL_FALSE, projectionMatrix.getPtr());

    // caméra pour le shader env
    loc = glGetUniformLocation(g_EnvShader.GetProgram(), "u_cameraPos");
    glUniform3f(loc, camX, camY, camZ);

    // lier la cubemap sur l’unité 3
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    loc = glGetUniformLocation(g_EnvShader.GetProgram(), "u_envMap");
    glUniform1i(loc, 3);

    glBindVertexArray(g_envModel.vao);
    glDrawElements(GL_TRIANGLES, g_envModel.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// --- Définitions des Callbacks GLFW --- 

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            g_isDragging = true;
            glfwGetCursorPos(window, &g_lastMouseX, &g_lastMouseY);
        } else if (action == GLFW_RELEASE) {
            g_isDragging = false;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (g_isDragging) {
        float dx = (float)(xpos - g_lastMouseX);
        float dy = (float)(ypos - g_lastMouseY);

        g_cameraYaw -= dx * 0.005f; // Sensibilité de la souris pour le lacet
        g_cameraPitch -= dy * 0.005f; // Sensibilité de la souris pour le tangage

        // Limiter le tangage pour éviter de basculer la caméra
        if (g_cameraPitch > 1.55f) g_cameraPitch = 1.55f; // Un peu moins de PI/2 (environ 88 degrés)
        if (g_cameraPitch < -1.55f) g_cameraPitch = -1.55f; // Un peu plus de -PI/2

        g_lastMouseX = xpos;
        g_lastMouseY = ypos;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    g_cameraDistance -= (float)yoffset * 0.5f; // Sensibilité du zoom
    if (g_cameraDistance < 0.5f) g_cameraDistance = 0.5f; // Distance minimale
    if (g_cameraDistance > 20.0f) g_cameraDistance = 20.0f; // Distance maximale
}

// --- Fin des Callbacks GLFW ---

void Terminate()
{
    // —–– Cube (g_mainModel + g_BasicShader)
    glDeleteBuffers(1, &g_mainModel.vbo);
    glDeleteBuffers(1, &g_mainModel.ibo);
    glDeleteVertexArrays(1, &g_mainModel.vao);
    g_BasicShader.Destroy();

    // —–– Pomme (g_secondModel + g_TextureShader)
    glDeleteBuffers(1, &g_secondModel.vbo);
    glDeleteBuffers(1, &g_secondModel.ibo);
    glDeleteVertexArrays(1, &g_secondModel.vao);
    glDeleteTextures(1, &secondTex);
    g_TextureShader.Destroy();
 
    // —–– OBJET ENV  
    glDeleteBuffers(1, &g_envModel.vbo);
    glDeleteBuffers(1, &g_envModel.ibo);
    glDeleteVertexArrays(1, &g_envModel.vao);
    glDeleteTextures(1, &envCubemap);
    g_EnvShader.Destroy();
}


int main(void)
{

    if (!glfwInit()){
        return -1;
    }
    
    
    
    // Request OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
    #endif

    /* Create a windowed mode window and its OpenGL context */
    g_window = glfwCreateWindow(640, 480, "Dragon", NULL, NULL);
    
    
    if (!g_window)
    {
        glfwTerminate();
        return -1;
    }
    
    /* Make the window's context current */
    glfwMakeContextCurrent(g_window);
    
    glfwSetWindowSizeCallback(g_window, window_size_callback);
    glfwSetMouseButtonCallback(g_window, mouse_button_callback);
    glfwSetCursorPosCallback(g_window, cursor_position_callback);
    glfwSetScrollCallback(g_window, scroll_callback);

    #ifdef _WIN32
    glewInit();
    #endif
    
    Initialise();
    

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(g_window))
    {
        /* Render here */
        Render();

        /* Swap front and back buffers */
        glfwSwapBuffers(g_window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    Terminate();
    glfwTerminate();
    return 0;
}
