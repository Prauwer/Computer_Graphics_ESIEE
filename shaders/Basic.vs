#version 330 core

in vec3 a_position;
in vec2 a_uv;
in vec3 a_color;

out vec2 v_uv;
out vec3 v_color;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
    v_uv = vec2(a_uv.s, 1.0 - a_uv.t);
    v_color = a_color;
}
