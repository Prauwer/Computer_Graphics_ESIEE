#version 330 core
in  vec3 v_dir;
out vec4 fragColor;
uniform samplerCube u_skybox;
void main() {
    fragColor = texture(u_skybox, v_dir);
}
