#version 330 core
in  vec3 v_normal;
out vec4 fragColor;

// <<< Ajout >>>
uniform vec3 u_color;

void main()
{
    // pour l’instant on ignore l’éclairage et on renvoie juste la couleur
    fragColor = vec4(u_color, 1.0);
}