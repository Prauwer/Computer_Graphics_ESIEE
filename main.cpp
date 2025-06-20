
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

// A simple struct to hold our loaded model's data
struct Model {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    int indexCount = 0;
};

Model g_mainModel; // Global model object


// Vertex du Dragon
struct Vertex {
    float position[3]; // x, y, z
    float normal[3]; // nx, ny, ny
    float uv[2]; // u, v

    bool operator==(const Vertex& other) const {
        return memcmp(this, &other, sizeof(Vertex)) == 0;
    }
};

// Hash function for Vertex, enabling its use in std::unordered_map
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
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    // UV attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
}



// // Removed old transformMatrixes function

//     // Angle de rotation en radians (45°)
//     const float theta = 0.0f * 3.14159f / 180.0f;

//     // Coefficients pour le scale
//     const float scaleX = 0.14f, scaleY = 0.14f, scaleZ = 0.14f;

//     // Valeurs de translation
//     const float tx = 0.0f, ty = -0.6f, tz = 0.0f;


//     // Matrice de scale en 4x4 (stockée en ordre colonne-major)
//     float scaleMatrix[16] = {
//         scaleX, 0.0f,   0.0f,   0.0f,
//         0.0f,   scaleY, 0.0f,   0.0f,
//         0.0f,   0.0f,   scaleZ, 0.0f,
//         0.0f,   0.0f,   0.0f,   1.0f
//     };

//     // Matrice de rotation autour de l'axe Z en 4x4 (colonne-major)
//     float rotationMatrix[16] = {
//         cos(theta),  sin(theta), 0.0f, 0.0f,
//         -sin(theta),  cos(theta), 0.0f, 0.0f,
//         0.0f,        0.0f,       1.0f, 0.0f,
//         0.0f,        0.0f,       0.0f, 1.0f
//     };

//     // Matrice de translation en 4x4 (colonne-major)
//     float translationMatrix[16] = {
//         1.0f, 0.0f, 0.0f, 0.0f,
//         0.0f, 1.0f, 0.0f, 0.0f,
//         0.0f, 0.0f, 1.0f, 0.0f,
//         tx,   ty,   tz,   1.0f
//     };

    
    
//     // Récupérer les localisations des variables uniformes dans le shader
//     GLuint scaleLoc = glGetUniformLocation(basicProgram, "uScale");
//     GLuint rotationLoc = glGetUniformLocation(basicProgram, "uRotation");
//     GLuint translationLoc = glGetUniformLocation(basicProgram, "uTranslation");

//     // Envoi des matrices aux uniformes du shader
//     glUniformMatrix4fv(scaleLoc, 1, GL_FALSE, scaleMatrix);
//     glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, rotationMatrix);
//     glUniformMatrix4fv(translationLoc, 1, GL_FALSE, translationMatrix);
// }

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

    // Matrice Modèle (transformations de l'objet)
    // Simplification pour le débogage : on utilise une matrice identité.
    // L'objet sera à l'origine, sans rotation ni scale.
        // On réintroduit une matrice de transformation pour ajuster la taille.
    // N'hésitez pas à changer la valeur de 'scaleFactor'.
    float scaleFactor = 1.0f; // Commencez avec 1.0 et ajustez si besoin
    mat4 modelMatrix = mat4::scale(scaleFactor, scaleFactor, scaleFactor);

    // Matrice Vue (position et orientation de la caméra)
    mat4 viewMatrix = mat4::lookAt(
        vec3(0.0f, 0.0f, 5.0f),    // Position de la caméra (reculée sur Z)
        vec3(0.0f, 0.0f, 0.0f),    // Point regardé (l'origine)
        vec3(0.0f, 1.0f, 0.0f)     // Vecteur "haut"
    );

    // Matrice Projection (perspective)
    mat4 projectionMatrix = mat4::perspective(45.0f * 3.14159f / 180.0f, // FOV en radians
                                              aspectRatio,
                                              0.1f,    // Near plane
                                              100.0f); // Far plane

    // Récupérer les localisations des uniformes
    GLint modelLoc = glGetUniformLocation(basicProgram, "u_model");
    GLint viewLoc = glGetUniformLocation(basicProgram, "u_view");
    GLint projectionLoc = glGetUniformLocation(basicProgram, "u_projection");

    // Envoyer les matrices au shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelMatrix.getPtr());
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix.getPtr());
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projectionMatrix.getPtr());

    // Application de la texture
    glUniform1i(glGetUniformLocation(g_BasicShader.GetProgram(), "u_texture"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_mainTex);
    
    // Étape e. Activation du VAO du modèle
    glBindVertexArray(g_mainModel.vao);

    // Étape f. Dessin du modèle
    glDrawElements(GL_TRIANGLES, g_mainModel.indexCount, GL_UNSIGNED_INT, 0);

    // Étape g. Désactivation du VAO
    glBindVertexArray(0);
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

// g++ main.cpp GLShader.cpp -o toto -lglfw3 -lglew32 -lopengl32 && ./toto