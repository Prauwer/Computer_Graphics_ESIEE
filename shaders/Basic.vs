#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

out vec2 v_uv;

// La matrice modèle reste une uniform individuelle
uniform mat4 u_model;

// Définition du bloc UBO partagé pour les matrices
layout (std140) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

void main()
{
    // On utilise les matrices du bloc UBO
    gl_Position = projection * view * u_model * vec4(a_position, 1.0);
    v_uv = vec2(a_uv.s, 1.0 - a_uv.t); // retournement vertical si besoin
}
