#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

// La matrice modèle reste une uniform individuelle
uniform mat4 u_model;

// Définition du bloc UBO partagé pour les matrices
layout (std140) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

out vec3 v_worldPos;
out vec3 v_worldNormal;

void main()
{
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    v_worldPos    = worldPos.xyz;
    v_worldNormal = mat3(u_model) * a_normal; 
    // On utilise les matrices du bloc UBO
    gl_Position   = projection * view * worldPos;
}
