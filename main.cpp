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
GLShader g_PhongShader;
GLFWwindow* g_window;

GLuint g_mainTex = 0;
GLuint secondTex = 0;
GLuint envCubemap = 0;
GLuint sphereCubemap = 0;
GLuint  skyboxVAO = 0, skyboxVBO = 0;
GLuint g_uboMatrices = 0;

// Variables pour la caméra orbitale
float g_cameraDistance = 5.0f;
float g_cameraYaw = 0.0f;
float g_cameraPitch = 0.0f;

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

struct UniformBlockMatrices {
    mat4 projection;
    mat4 view;
};

Model g_mainModel;
Model g_secondModel;
Model g_envModel;

struct Vertex {
    float position[3];
    float normal[3];
    float uv[2];
    bool operator==(const Vertex& other) const {
        return memcmp(this, &other, sizeof(Vertex)) == 0;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
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

void window_size_callback(GLFWwindow* window, int width, int height) {}

GLuint loadTexture(const char* path) {
    int w, h, n;
    unsigned char* data = stbi_load(path, &w, &h, &n, STBI_rgb_alpha);
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    // <<< CORRECTION sRGB 1/2 >>>
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return tex;
}

void layout() {
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
}

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

GLuint loadCubemap(const std::vector<std::string>& faces) {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);
    int w, h, n;
    for (GLuint i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &n, STBI_rgb_alpha);
        if (data) {
            // Note: Les cubemaps pour l'éclairage ne sont généralement pas en sRGB
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cerr << "Cubemap load failed: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return texID;
}

bool Initialise() {
    g_BasicShader.LoadVertexShader("shaders/basic.vs");
    g_BasicShader.LoadFragmentShader("shaders/basic.fs");
    g_BasicShader.Create();
    glUniformBlockBinding(g_BasicShader.GetProgram(), glGetUniformBlockIndex(g_BasicShader.GetProgram(), "Matrices"), 0);

    g_TextureShader.LoadVertexShader("shaders/texture.vs");
    g_TextureShader.LoadFragmentShader("shaders/texture.fs");
    g_TextureShader.Create();
    glUniformBlockBinding(g_TextureShader.GetProgram(), glGetUniformBlockIndex(g_TextureShader.GetProgram(), "Matrices"), 0);

    g_EnvShader.LoadVertexShader("shaders/env.vs");
    g_EnvShader.LoadFragmentShader("shaders/env.fs");
    g_EnvShader.Create();
    glUniformBlockBinding(g_EnvShader.GetProgram(), glGetUniformBlockIndex(g_EnvShader.GetProgram(), "Matrices"), 0);

    g_SkyboxShader.LoadVertexShader("shaders/skybox.vs");
    g_SkyboxShader.LoadFragmentShader("shaders/skybox.fs");
    g_SkyboxShader.Create();
    // Pas de binding UBO pour la nouvelle skybox

    g_PhongShader.LoadVertexShader("shaders/phong.vs");
    g_PhongShader.LoadFragmentShader("shaders/phong.fs");
    g_PhongShader.Create();
    glUniformBlockBinding(g_PhongShader.GetProgram(), glGetUniformBlockIndex(g_PhongShader.GetProgram(), "Matrices"), 0);

    glGenBuffers(1, &g_uboMatrices);
    glBindBuffer(GL_UNIFORM_BUFFER, g_uboMatrices);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBlockMatrices), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_uboMatrices);

    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    #ifdef WIN32
    wglSwapIntervalEXT(1);
    #endif

    secondTex = loadTexture("assets/3DApple002_SQ-1K-PNG/3DApple002_SQ-1K-PNG_Color.png"); 
    try {
        g_mainModel = loadObjModel("assets/cube.obj");
        g_secondModel = loadObjModel("assets/3DApple002_SQ-1K-PNG/3DApple002_SQ-1K-PNG.obj");
        g_envModel = loadObjModel("assets/sphere.obj");
    } catch (const std::exception& e) {
        std::cerr << "Error loading model: " << e.what() << std::endl;
        return false;
    }
    envCubemap = loadCubemap({ "assets/cloudy/bluecloud_rt.jpg", "assets/cloudy/bluecloud_lf.jpg", "assets/cloudy/bluecloud_up.jpg", "assets/cloudy/bluecloud_dn.jpg", "assets/cloudy/bluecloud_ft.jpg", "assets/cloudy/bluecloud_bk.jpg" });
    sphereCubemap = loadCubemap({ "assets/Yokohama3/posx.jpg", "assets/Yokohama3/negx.jpg", "assets/Yokohama3/posy.jpg", "assets/Yokohama3/negy.jpg", "assets/Yokohama3/posz.jpg", "assets/Yokohama3/negz.jpg" });

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    // <<< CORRECTION sRGB 2/2 >>>
    glEnable(GL_FRAMEBUFFER_SRGB);
    return true;
}

void Render()
{
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.75f, 0.75f, 0.75f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspectRatio = (height > 0) ? ((float)width / height) : 1.0f;
    float camX = g_cameraDistance * sin(g_cameraYaw) * cos(g_cameraPitch);
    float camY = g_cameraDistance * sin(g_cameraPitch);
    float camZ = g_cameraDistance * cos(g_cameraYaw) * cos(g_cameraPitch);
    mat4 viewMatrix = mat4::lookAt(vec3(camX, camY, camZ), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    mat4 projectionMatrix = mat4::perspective(45.0f * 3.14159f / 180.0f, aspectRatio, 0.01f, 100.0f);
    
    // Mise à jour de l'UBO (une seule fois pour tous les objets)
    UniformBlockMatrices uboData;
    uboData.projection = projectionMatrix;
    uboData.view = viewMatrix;
    glBindBuffer(GL_UNIFORM_BUFFER, g_uboMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UniformBlockMatrices), &uboData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // 1) DESSIN DU SKYBOX 
    glDepthFunc(GL_LEQUAL); 
    glUseProgram(g_SkyboxShader.GetProgram());
    
    mat4 viewNoTrans = viewMatrix;
    float* p = const_cast<float*>(viewNoTrans.getPtr());
    p[12] = 0.0f; p[13] = 0.0f; p[14] = 0.0f;

    glUniformMatrix4fv(glGetUniformLocation(g_SkyboxShader.GetProgram(), "view"), 1, GL_FALSE, viewNoTrans.getPtr());
    glUniformMatrix4fv(glGetUniformLocation(g_SkyboxShader.GetProgram(), "projection"), 1, GL_FALSE, projectionMatrix.getPtr());
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glUniform1i(glGetUniformLocation(g_SkyboxShader.GetProgram(), "u_skybox"), 0);
    
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    glDepthFunc(GL_LESS); 
    
    // 2) DESSIN DU CUBE (AVEC ÉCLAIRAGE PHONG)
    auto phongProgram = g_PhongShader.GetProgram();
    glUseProgram(phongProgram); 
    float rotationXAngle = 20.0f * 3.1415926535f / 180.0f;
    mat4 modelCube = mat4::translate(-2.0f, 0.0f, 0.0f) * mat4::rotateX(rotationXAngle) * mat4::scale(1.0f, 1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(phongProgram, "u_model"), 1, GL_FALSE, modelCube.getPtr());
    glUniform3f(glGetUniformLocation(phongProgram, "u_objectColor"), 0.0f, 0.0f, 1.0f);
    glUniform3f(glGetUniformLocation(phongProgram, "u_lightColor"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(phongProgram, "u_lightPos"), 0.0f, 5.0f, 2.0f);
    glUniform3f(glGetUniformLocation(phongProgram, "u_viewPos"), camX, camY, camZ);
    glUniform1f(glGetUniformLocation(phongProgram, "u_shininess"), 32.0f);
    glBindVertexArray(g_mainModel.vao);
    glDrawElements(GL_TRIANGLES, g_mainModel.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // 3) DESSIN DE LA POMME
    auto secondProgram = g_TextureShader.GetProgram();
    glUseProgram(secondProgram);
    float rotationXAngleApple = 5.0f * 3.1415926535f / 180.0f;
    mat4 modelApple = mat4::translate( 2.0f, -0.5f, 0.0f) * mat4::rotateX(rotationXAngleApple) * mat4::scale(20.0f, 20.0f, 20.0f);
    glUniformMatrix4fv(glGetUniformLocation(secondProgram, "u_model"), 1, GL_FALSE, modelApple.getPtr());
    glUniform1i(glGetUniformLocation(secondProgram, "u_texture"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, secondTex);
    glBindVertexArray(g_secondModel.vao);
    glDrawElements(GL_TRIANGLES, g_secondModel.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // 4) DESSIN DE LA SPHÈRE ENVMAP
    glUseProgram(g_EnvShader.GetProgram());
    mat4 modelEnv = mat4::translate(0.0f, 0.0f, 0.0f) * mat4::scale(.8f, .8f, .8f);
    glUniformMatrix4fv(glGetUniformLocation(g_EnvShader.GetProgram(), "u_model"), 1, GL_FALSE, modelEnv.getPtr());
    glUniform3f(glGetUniformLocation(g_EnvShader.GetProgram(), "u_cameraPos"), camX, camY, camZ);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sphereCubemap);
    glUniform1i(glGetUniformLocation(g_EnvShader.GetProgram(), "u_envMap"), 3);
    glBindVertexArray(g_envModel.vao);
    glDrawElements(GL_TRIANGLES, g_envModel.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

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
        g_cameraYaw -= dx * 0.005f;
        g_cameraPitch -= dy * 0.005f;
        if (g_cameraPitch > 1.55f) g_cameraPitch = 1.55f;
        if (g_cameraPitch < -1.55f) g_cameraPitch = -1.55f;
        g_lastMouseX = xpos;
        g_lastMouseY = ypos;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    g_cameraDistance -= (float)yoffset * 0.5f;
    if (g_cameraDistance < 0.5f) g_cameraDistance = 0.5f;
    if (g_cameraDistance > 20.0f) g_cameraDistance = 20.0f;
}

void Terminate()
{
    glDeleteBuffers(1, &g_mainModel.vbo);
    glDeleteBuffers(1, &g_mainModel.ibo);
    glDeleteVertexArrays(1, &g_mainModel.vao);
    g_BasicShader.Destroy();
    glDeleteBuffers(1, &g_secondModel.vbo);
    glDeleteBuffers(1, &g_secondModel.ibo);
    glDeleteVertexArrays(1, &g_secondModel.vao);
    glDeleteTextures(1, &secondTex);
    g_TextureShader.Destroy();
    glDeleteBuffers(1, &g_envModel.vbo);
    glDeleteBuffers(1, &g_envModel.ibo);
    glDeleteVertexArrays(1, &g_envModel.vao);
    glDeleteTextures(1, &envCubemap);
    g_EnvShader.Destroy();
    g_PhongShader.Destroy();
    glDeleteBuffers(1, &g_uboMatrices);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
}

int main(void)
{
    if (!glfwInit()){
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    g_window = glfwCreateWindow(640, 480, "Dragon", NULL, NULL);
    if (!g_window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(g_window);
    glfwSetWindowSizeCallback(g_window, window_size_callback);
    glfwSetMouseButtonCallback(g_window, mouse_button_callback);
    glfwSetCursorPosCallback(g_window, cursor_position_callback);
    glfwSetScrollCallback(g_window, scroll_callback);

    #ifdef _WIN32
    glewInit();
    #endif
    
    Initialise();
    
    while (!glfwWindowShouldClose(g_window)) {
        Render();
        glfwSwapBuffers(g_window);
        glfwPollEvents();
    }

    Terminate();
    glfwTerminate();
    return 0;
}
