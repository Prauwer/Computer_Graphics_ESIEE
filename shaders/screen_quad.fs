#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int u_postProcessEffect; // 0: None, 1: Grayscale, 2: Invert, 3: Sepia

void main()
{
    vec4 color = texture(screenTexture, TexCoords);
    vec3 processedColor = color.rgb;

    if (u_postProcessEffect == 1) { // Grayscale
        // Calcule la valeur de gris (luminance)
        // Utilise une formule courante pour convertir RGB en niveaux de gris,
        // en tenant compte de la perception humaine des couleurs.
        float grayscale = dot(processedColor, vec3(0.2126, 0.7152, 0.0722));
        processedColor = vec3(grayscale);
    } else if (u_postProcessEffect == 2) { // Invert Colors
        processedColor = vec3(1.0 - processedColor);
    } else if (u_postProcessEffect == 3) { // Sepia
        processedColor = vec3(
            dot(processedColor, vec3(0.393, 0.769, 0.189)),
            dot(processedColor, vec3(0.349, 0.686, 0.168)),
            dot(processedColor, vec3(0.272, 0.534, 0.131))
        );
        // Optionnel: clamp pour s'assurer que les valeurs restent entre 0 et 1
        processedColor = clamp(processedColor, 0.0, 1.0);
    }
    // Si u_postProcessEffect est 0, aucun effet n'est appliqué (processedColor reste la couleur originale)

    FragColor = vec4(processedColor, color.a);
}
