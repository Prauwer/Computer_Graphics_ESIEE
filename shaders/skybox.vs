#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 v_TexCoords;

uniform mat4 projection;
uniform mat4 view; // Matrice de vue spéciale (sans translation)

void main()
{
    v_TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    // Astuce pour forcer la skybox à être toujours au fond
    gl_Position = pos.xyww;
}