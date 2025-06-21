#version 330 core

in  vec3 v_worldPos;
in  vec3 v_worldNormal;

uniform samplerCube u_envMap;
uniform vec3        u_cameraPos;

out vec4 fragColor;

void main()
{
    // vecteur incident : de la surface vers la caméra
    vec3 I = normalize(v_worldPos - u_cameraPos);
    // vecteur réfléchi
    vec3 R = reflect(I, normalize(v_worldNormal));
    fragColor = texture(u_envMap, R);
}
