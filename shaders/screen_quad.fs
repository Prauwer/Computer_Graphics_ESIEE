#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int u_postProcessEffect; // 0: None, 1: Grayscale, 2: Invert, 3: Sepia
uniform float u_saturation;      // Saturation uniform
uniform float u_contrast;        // Contrast uniform

void main()
{
    vec4 color = texture(screenTexture, TexCoords);
    vec3 processedColor = color.rgb;

    // Apply selected post-process effect first
    if (u_postProcessEffect == 1) { // Grayscale
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
        processedColor = clamp(processedColor, 0.0, 1.0); // Clamp values to stay in [0,1] range
    }

    // Apply saturation adjustment
    // Formula: mix(luminance, color, saturation)
    float luminance = dot(processedColor, vec3(0.2126, 0.7152, 0.0722));
    processedColor = mix(vec3(luminance), processedColor, u_saturation);

    // Apply contrast adjustment
    // Formula: (color - 0.5) * contrast + 0.5
    processedColor = ((processedColor - 0.5) * u_contrast) + 0.5;
    processedColor = clamp(processedColor, 0.0, 1.0); // Clamp values to stay in [0,1] range

    FragColor = vec4(processedColor, color.a);
}
