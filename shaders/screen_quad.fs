#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    // Échantillonne la couleur de la texture du FBO
    vec4 color = texture(screenTexture, TexCoords);

    // Calcule la valeur de gris (luminance)
    // Utilise une formule courante pour convertir RGB en niveaux de gris,
    // en tenant compte de la perception humaine des couleurs.
    float grayscale = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));

    // Applique la couleur en niveaux de gris, en gardant l'alpha original
    FragColor = vec4(vec3(grayscale), color.a);

    // Pour un effet d'inversion des couleurs simple, vous pourriez faire :
    // FragColor = vec4(vec3(1.0 - color.rgb), color.a);
}
