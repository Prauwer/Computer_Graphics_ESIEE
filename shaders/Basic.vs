#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;

out vec2 v_uv;


uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
    v_uv = vec2(a_uv.s, 1.0 - a_uv.t); // Inverser la coordonnée V de la texture (compatibilité avec certains formats d'image)
}
