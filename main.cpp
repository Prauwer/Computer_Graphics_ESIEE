
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
GLFWwindow* g_window;
GLuint g_mainTex = 0; // Global texture object

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


bool Initialise()
{
    g_BasicShader.LoadVertexShader("shaders/basic.vs");
    g_BasicShader.LoadFragmentShader("shaders/basic.fs");
    g_BasicShader.Create();
    
    #ifdef WIN32
    wglSwapIntervalEXT(1);
    #endif

    g_mainTex = loadTexture("assets/dragon.png"); // Assuming texture is in assets/

    try {
        g_mainModel = loadObjModel("assets/apple.obj"); // Assuming model is in assets/
    } catch (const std::exception& e) {
        std::cerr << "Error loading model: " << e.what() << std::endl;
        return false;
    }

    

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    return true;
}

void Terminate()
{
    glDeleteBuffers(1, &g_mainModel.vbo);
    glDeleteBuffers(1, &g_mainModel.ibo);
    glDeleteVertexArrays(1, &g_mainModel.vao);
    glDeleteTextures(1, &g_mainTex);
    g_BasicShader.Destroy();
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

    // Étape c. Spécification du shader program à utiliser
    auto basicProgram = g_BasicShader.GetProgram();
    glUseProgram(basicProgram);

    // Étape d. Configuration des matrices MVP (Model-View-Projection)
    // int width, height; // Redundant declaration, already declared above in this function
    glfwGetFramebufferSize(g_window, &width, &height);
    float aspectRatio = (height > 0) ? ((float)width / height) : 1.0f;

    // 1. Matrice Modèle : position, rotation et échelle de l'objet dans le monde.
    // Simplification pour le débogage : on utilise une matrice identité.
    // L'objet sera à l'origine, sans rotation ni scale.
        // On réintroduit une matrice de transformation pour ajuster la taille.
    // N'hésitez pas à changer la valeur de 'scaleFactor'.
    float scaleFactor = 1.0f; // Taille normale
    float rotationXAngle = 20.0f * 3.1415926535f / 180.0f; // 20 degrés en radians

    mat4 scaleMatrix = mat4::scale(scaleFactor, scaleFactor, scaleFactor);
    mat4 rotationXMatrix = mat4::rotateX(rotationXAngle);
    mat4 modelMatrix = rotationXMatrix * scaleMatrix; // Appliquer la rotation puis la mise à l'échelle

    // 2. Matrice Vue : positionne et oriente la caméra.
    // Calcul de la position de la caméra en coordonnées cartésiennes à partir des coordonnées sphériques
    float camX = g_cameraDistance * sin(g_cameraYaw) * cos(g_cameraPitch);
    float camY = g_cameraDistance * sin(g_cameraPitch);
    float camZ = g_cameraDistance * cos(g_cameraYaw) * cos(g_cameraPitch);

    mat4 viewMatrix = mat4::lookAt(
        vec3(camX, camY, camZ),    // Position de la caméra calculée
        vec3(0.0f, 0.0f, 0.0f),    // Point regardé (l'origine)
        vec3(0.0f, 1.0f, 0.0f)     // Vecteur "haut"
    );

    // 3. Matrice Projection : définit le type de projection (perspective) et le champ de vision.
    mat4 projectionMatrix = mat4::perspective(45.0f * 3.14159f / 180.0f, // FOV en radians
                                              aspectRatio,
                                              0.1f,    // Near plane
                                              100.0f); // Far plane

    // Envoyer les matrices (Modèle, Vue, Projection) au shader
    GLint modelLoc = glGetUniformLocation(basicProgram, "u_model");
    GLint viewLoc = glGetUniformLocation(basicProgram, "u_view");
    GLint projectionLoc = glGetUniformLocation(basicProgram, "u_projection");

    // Envoyer les matrices au shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelMatrix.getPtr());
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix.getPtr());
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projectionMatrix.getPtr());

    // Lier la texture à l'unité de texture 0
    glUniform1i(glGetUniformLocation(g_BasicShader.GetProgram(), "u_texture"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_mainTex);
    
    // Activer le VAO du modèle pour le dessin
    glBindVertexArray(g_mainModel.vao);

    // Dessiner le modèle en utilisant les indices
    glDrawElements(GL_TRIANGLES, g_mainModel.indexCount, GL_UNSIGNED_INT, 0);

    // Désactiver le VAO pour éviter les modifications accidentelles
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
