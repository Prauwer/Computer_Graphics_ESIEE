#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

// Matrices de transformation
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

// Données à passer au Fragment Shader
out vec3 v_worldPos;
out vec3 v_worldNormal;

void main()
{
    // Calcule la position du sommet dans l'espace monde
    v_worldPos = vec3(u_model * vec4(a_position, 1.0));
    
    // Correction: Utilisation d'une transformation de normale plus simple et robuste
    // pour éviter les problèmes liés à l'inversion de matrice.
    v_worldNormal = normalize(mat3(u_model) * a_normal);
    
    // Position finale du sommet pour le rendu
    gl_Position = u_projection * u_view * vec4(v_worldPos, 1.0);
}