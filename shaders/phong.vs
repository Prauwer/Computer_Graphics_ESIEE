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

// Données à passer au Fragment Shader
out vec3 v_worldPos;
out vec3 v_worldNormal;

void main()
{
    // Calcule la position du sommet dans l'espace monde
    v_worldPos = vec3(u_model * vec4(a_position, 1.0));
    
    // Correction: Utilisation d'une transformation de normale plus simple et robuste
    v_worldNormal = normalize(mat3(u_model) * a_normal);
    
    // Position finale du sommet pour le rendu en utilisant les matrices de l'UBO
    gl_Position = projection * view * vec4(v_worldPos, 1.0);
}
