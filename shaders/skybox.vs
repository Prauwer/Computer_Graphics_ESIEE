#version 330 core
layout(location = 0) in vec3 a_pos;
out vec3 v_dir;
uniform mat4 u_view;       // view sans translation
uniform mat4 u_projection;
void main() {
    v_dir = a_pos;
    gl_Position = u_projection * u_view * vec4(a_pos, 1.0);
}
