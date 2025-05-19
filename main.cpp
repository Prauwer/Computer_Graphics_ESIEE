#define GL_SILENCE_DEPRECATION

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
#include "GLShader.h"
#include "dragonData.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint VBO;
GLuint IBO;
GLuint VAO;
GLuint dragonTex;
GLShader g_BasicShader;
GLFWwindow* g_window;

// Vertex du Dragon
struct Vertex {
    float position[3]; // x, y, z
    float normal[3]; // nx, ny, ny
    float uv[2]; // u, v
};

void window_size_callback(GLFWwindow* window, int width, int height)
{
    glfwSetWindowSize(window, 640, 480);
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

void layout(uint32_t basicProgram) {
    // Étape e. Spécification des attributs de vertex
    GLint positionAttrib = glGetAttribLocation(basicProgram, "a_position");
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)0);
    glEnableVertexAttribArray(positionAttrib);

    // Étape f. Spécification des attributs de couleur
    GLint colorAttrib = glGetAttribLocation(basicProgram, "a_color");
    glVertexAttribPointer(colorAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(colorAttrib);

    // Étape f. Spécification des attributs de texture
    GLint uvLoc = glGetAttribLocation(basicProgram, "a_uv");
    glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(uvLoc);
}

void initVBO(){
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(DragonVertices), DragonVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void initIBO(){
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(DragonIndices), DragonIndices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void initVAO() {
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);


    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

    layout(g_BasicShader.GetProgram());

    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void transformMatrixes(uint32_t basicProgram) {
    // Angle de rotation en radians (45°)
    const float theta = 0.0f * 3.14159f / 180.0f;

    // Coefficients pour le scale
    const float scaleX = 0.14f, scaleY = 0.14f, scaleZ = 0.14f;

    // Valeurs de translation
    const float tx = 0.0f, ty = -0.6f, tz = 0.0f;


    // Matrice de scale en 4x4 (stockée en ordre colonne-major)
    float scaleMatrix[16] = {
        scaleX, 0.0f,   0.0f,   0.0f,
        0.0f,   scaleY, 0.0f,   0.0f,
        0.0f,   0.0f,   scaleZ, 0.0f,
        0.0f,   0.0f,   0.0f,   1.0f
    };

    // Matrice de rotation autour de l'axe Z en 4x4 (colonne-major)
    float rotationMatrix[16] = {
        cos(theta),  sin(theta), 0.0f, 0.0f,
        -sin(theta),  cos(theta), 0.0f, 0.0f,
        0.0f,        0.0f,       1.0f, 0.0f,
        0.0f,        0.0f,       0.0f, 1.0f
    };

    // Matrice de translation en 4x4 (colonne-major)
    float translationMatrix[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        tx,   ty,   tz,   1.0f
    };

    
    
    // Récupérer les localisations des variables uniformes dans le shader
    GLuint scaleLoc = glGetUniformLocation(basicProgram, "uScale");
    GLuint rotationLoc = glGetUniformLocation(basicProgram, "uRotation");
    GLuint translationLoc = glGetUniformLocation(basicProgram, "uTranslation");

    // Envoi des matrices aux uniformes du shader
    glUniformMatrix4fv(scaleLoc, 1, GL_FALSE, scaleMatrix);
    glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, rotationMatrix);
    glUniformMatrix4fv(translationLoc, 1, GL_FALSE, translationMatrix);
}

bool Initialise()
{
    g_BasicShader.LoadVertexShader("basic.vs");
    g_BasicShader.LoadFragmentShader("basic.fs");
    g_BasicShader.Create();
    // cette fonction est spécifique à Windows et permet d’activer (1) ou non (0)
    // la synchronization vertical. Elle necessite l’include wglew.h
    #ifdef WIN32
    wglSwapIntervalEXT(1);
    #endif

    // charge la texture dragon.png
    dragonTex = loadTexture("dragon.png");

    // Active le depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    

    initVBO();
    initIBO();
    initVAO();

    glDisable(GL_CULL_FACE);

    return true;
}

void Terminate()
{
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &IBO);
    glDeleteVertexArrays(1, &VAO);
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

    // APPLICATION DES MATRICES DE TRANSLATION
    transformMatrixes(basicProgram);

    // Application de la texture
    glUseProgram(g_BasicShader.GetProgram());
    glUniform1i(glGetUniformLocation(g_BasicShader.GetProgram(), "uTexture"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, dragonTex);
    
    // Étape d. Activation du VAO
    glBindVertexArray(VAO);

    // Étape g. Dessin du dragon
    int indexCount = sizeof(DragonIndices) / sizeof(DragonIndices[0]);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);

    // Étape h. Désactivation du VAO
    glBindVertexArray(0);
}



int main(void)
{
    /* Initialize the library */
    if (!glfwInit())
        return -1;

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

    #ifdef __APPLE__
    glfwInit();
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