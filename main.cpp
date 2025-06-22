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
#include "mat4.h"
#include "GLShader.h"
#include <vector>
#include <unordered_map>
#include <string>

// --- ImGui includes ---
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
// ----------------------

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

// Global variables
GLShader g_BasicShader;
GLShader g_TextureShader;
GLShader g_EnvShader;
GLShader g_SkyboxShader;
GLShader g_PhongShader;
GLShader g_ScreenQuadShader;
GLFWwindow* g_window;

GLuint g_mainTex = 0;
GLuint secondTex = 0;
GLuint envCubemap = 0;
GLuint sphereCubemap = 0;
GLuint skyboxVAO = 0, skyboxVBO = 0;
GLuint g_uboMatrices = 0;

// FBO related variables
GLuint g_fbo = 0;
GLuint g_fboTexture = 0;
GLuint g_rboDepthStencil = 0; // Renderbuffer for depth and stencil

// Screen quad variables
GLuint g_screenQuadVAO = 0;
GLuint g_screenQuadVBO = 0;

// FBO dimensions (can be different from window dimensions)
const int FBO_WIDTH = 1024;
const int FBO_HEIGHT = 768;

// Variables pour la caméra orbitale
float g_cameraDistance = 5.0f;
float g_cameraYaw = 0.0f;
float g_cameraPitch = 0.0f;

bool g_isDragging = false;
double g_lastMouseX = 0.0;
double g_lastMouseY = 0.0;

// --- ImGui Post-processing state ---
int g_selectedPostProcessEffect = 0; // 0: None, 1: Grayscale, 2: Invert, 3: Sepia
float g_saturation = 1.0f; // New: Saturation control (1.0 for original)
float g_contrast = 1.0f;   // New: Contrast control (1.0 for original)
// -----------------------------------

// Callback functions for GLFW
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void char_callback(GLFWwindow* window, unsigned int c);


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

void window_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

GLuint loadTexture(const char* path) {
    int w, h, n;
    unsigned char* data = stbi_load(path, &w, &h, &n, STBI_rgb_alpha);
    if (!data) {
        return 0;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str());
    
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
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
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

    g_TextureShader.LoadVertexShader("shaders/texture.vs");
    g_TextureShader.LoadFragmentShader("shaders/texture.fs");
    g_TextureShader.Create();

    g_EnvShader.LoadVertexShader("shaders/env.vs");
    g_EnvShader.LoadFragmentShader("shaders/env.fs");
    g_EnvShader.Create();

    g_SkyboxShader.LoadVertexShader("shaders/skybox.vs");
    g_SkyboxShader.LoadFragmentShader("shaders/skybox.fs");
    g_SkyboxShader.Create();

    g_PhongShader.LoadVertexShader("shaders/phong.vs");
    g_PhongShader.LoadFragmentShader("shaders/phong.fs");
    g_PhongShader.Create();
    
    // Screen quad shader setup
    g_ScreenQuadShader.LoadVertexShader("shaders/screen_quad.vs");
    g_ScreenQuadShader.LoadFragmentShader("shaders/screen_quad.fs");
    g_ScreenQuadShader.Create();

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

    g_mainModel = loadObjModel("assets/cube.obj");
    g_secondModel = loadObjModel("assets/3DApple002_SQ-1K-PNG/3DApple002_SQ-1K-PNG.obj");
    g_envModel = loadObjModel("assets/sphere.obj");
    
    envCubemap = loadCubemap({ "assets/cloudy/bluecloud_rt.jpg", "assets/cloudy/bluecloud_lf.jpg", "assets/cloudy/bluecloud_up.jpg", "assets/cloudy/bluecloud_dn.jpg", "assets/cloudy/bluecloud_ft.jpg", "assets/cloudy/bluecloud_bk.jpg" });
    sphereCubemap = loadCubemap({ "assets/Yokohama3/posx.jpg", "assets/Yokohama3/negx.jpg", "assets/Yokohama3/posy.jpg", "assets/Yokohama3/negy.jpg", "assets/Yokohama3/posz.jpg", "assets/Yokohama3/negz.jpg" });

    // FBO setup
    glGenFramebuffers(1, &g_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);

    // Create a color attachment texture
    glGenTextures(1, &g_fboTexture);
    glBindTexture(GL_TEXTURE_2D, g_fboTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FBO_WIDTH, FBO_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Prevent sampling outside bounds
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Prevent sampling outside bounds
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_fboTexture, 0);

    // Create a renderbuffer object for depth and stencil attachment
    glGenRenderbuffers(1, &g_rboDepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, g_rboDepthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, FBO_WIDTH, FBO_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g_rboDepthStencil);

    glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind back to default framebuffer

    // Screen quad setup
    float quadVertices[] = {
        // positions   // texture Coords
        -1.0f,  1.0f,  0.0f, 1.0f, // top-left
        -1.0f, -1.0f,  0.0f, 0.0f, // bottom-left
         1.0f, -1.0f,  1.0f, 0.0f, // bottom-right

        -1.0f,  1.0f,  0.0f, 1.0f, // top-left
         1.0f, -1.0f,  1.0f, 0.0f, // bottom-right
         1.0f,  1.0f,  1.0f, 1.0f  // top-right
    };
    glGenVertexArrays(1, &g_screenQuadVAO);
    glGenBuffers(1, &g_screenQuadVBO);
    glBindVertexArray(g_screenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_screenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0); // Position
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float))); // Texture coordinates
    glBindVertexArray(0);


    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glEnable(GL_FRAMEBUFFER_SRGB); // Enable sRGB for correct color space handling

    // --- ImGui Initialization ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends - IMPORTANT: Pass false here to manually forward events
    ImGui_ImplGlfw_InitForOpenGL(g_window, false);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    // ----------------------------

    return true;
}

void Render()
{
    // --- ImGui New Frame ---
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // --- ImGui UI for Post-processing ---
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver); // Position (x, y) et condition
    ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver); // Taille (largeur, hauteur) et condition
    ImGui::Begin("Options de Post-traitement");
    ImGui::Text("Choisissez un effet :");
    ImGui::RadioButton("Aucun", &g_selectedPostProcessEffect, 0);
    ImGui::RadioButton("Niveaux de gris", &g_selectedPostProcessEffect, 1);
    ImGui::RadioButton("Inverser couleurs", &g_selectedPostProcessEffect, 2);
    ImGui::RadioButton("Sépia", &g_selectedPostProcessEffect, 3);
    
    ImGui::Separator(); // Add a separator for better visual organization
    ImGui::Text("Colorimétrie :");
    // Sliders for saturation and contrast
    ImGui::SliderFloat("Saturation", &g_saturation, 0.0f, 2.0f, "%.3f"); // Range from 0.0 (desaturated) to 2.0 (super saturated)
    ImGui::SliderFloat("Contraste", &g_contrast, 0.0f, 2.0f, "%.3f");   // Range from 0.0 (no contrast) to 2.0 (high contrast)

    ImGui::End();
    // ------------------------------------

    // --- Pass 1: Render scene to FBO ---
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glViewport(0, 0, FBO_WIDTH, FBO_HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspectRatio = (FBO_HEIGHT > 0) ? ((float)FBO_WIDTH / FBO_HEIGHT) : 1.0f;
    float camX = g_cameraDistance * sin(g_cameraYaw) * cos(g_cameraPitch);
    float camY = g_cameraDistance * sin(g_cameraPitch);
    float camZ = g_cameraDistance * cos(g_cameraYaw) * cos(g_cameraPitch);
    mat4 viewMatrix = mat4::lookAt(vec3(camX, camY, camZ), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    mat4 projectionMatrix = mat4::perspective(45.0f * 3.14159f / 180.0f, aspectRatio, 0.01f, 100.0f);
    
    // Mise à jour de l'UBO pour le FBO rendering
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

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind back to default framebuffer


    // --- Pass 2: Render FBO texture to screen ---
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.75f, 0.75f, 0.75f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST); // Disable depth testing for 2D quad

    glUseProgram(g_ScreenQuadShader.GetProgram());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_fboTexture);
    glUniform1i(glGetUniformLocation(g_ScreenQuadShader.GetProgram(), "screenTexture"), 0);
    glUniform1i(glGetUniformLocation(g_ScreenQuadShader.GetProgram(), "u_postProcessEffect"), g_selectedPostProcessEffect);
    // New: Pass saturation and contrast uniforms
    glUniform1f(glGetUniformLocation(g_ScreenQuadShader.GetProgram(), "u_saturation"), g_saturation);
    glUniform1f(glGetUniformLocation(g_ScreenQuadShader.GetProgram(), "u_contrast"), g_contrast);

    glBindVertexArray(g_screenQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6); // Draw the quad
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST); // Re-enable depth testing

    // --- ImGui Rendering ---
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    // -----------------------
}

// All custom GLFW callbacks explicitly forward to ImGui backend and then check if ImGui wants to capture input
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }
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
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }
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
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }
    g_cameraDistance -= (float)yoffset * 0.5f;
    if (g_cameraDistance < 0.5f) g_cameraDistance = 0.5f;
    if (g_cameraDistance > 20.0f) g_cameraDistance = 20.0f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

void char_callback(GLFWwindow* window, unsigned int c) {
    ImGui_ImplGlfw_CharCallback(window, c);
}

void Terminate()
{
    // --- ImGui Shutdown ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // ----------------------

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

    // FBO and screen quad cleanup
    glDeleteFramebuffers(1, &g_fbo);
    glDeleteTextures(1, &g_fboTexture);
    glDeleteRenderbuffers(1, &g_rboDepthStencil);
    g_ScreenQuadShader.Destroy();
    glDeleteVertexArrays(1, &g_screenQuadVAO);
    glDeleteBuffers(1, &g_screenQuadVBO);
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

    g_window = glfwCreateWindow(FBO_WIDTH, FBO_HEIGHT, "Projet Computer Graphics", NULL, NULL);
    if (!g_window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(g_window);

    // --- Set GLFW callbacks to also be handled by ImGui ---
    glfwSetWindowSizeCallback(g_window, window_size_callback);
    glfwSetMouseButtonCallback(g_window, mouse_button_callback);
    glfwSetCursorPosCallback(g_window, cursor_position_callback);
    glfwSetScrollCallback(g_window, scroll_callback);
    glfwSetKeyCallback(g_window, key_callback);
    glfwSetCharCallback(g_window, char_callback);
    // ----------------------------------------------------

    #ifdef _WIN32
    glewInit();
    #endif
    
    if (!Initialise()) {
        Terminate();
        glfwTerminate();
        return -1;
    }
    
    while (!glfwWindowShouldClose(g_window)) {
        Render();
        glfwSwapBuffers(g_window);
        glfwPollEvents();
    }

    Terminate();
    glfwTerminate();
    return 0;
}
