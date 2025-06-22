#version 330 core
out vec4 fragColor;

// Données reçues du Vertex Shader
in vec3 v_worldPos;
in vec3 v_worldNormal;

// Propriétés de la lumière
uniform vec3 u_lightPos;     // Position de la lumière dans l'espace monde
uniform vec3 u_lightColor;

// Propriétés du matériau de l'objet
uniform vec3 u_objectColor;
uniform float u_shininess;   // Exposant pour la brillance spéculaire

// Position de la caméra
uniform vec3 u_viewPos;      // Position de la caméra dans l'espace monde

void main()
{
    // --- 1. Composante Ambiante ---
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * u_lightColor;
    
    // --- 2. Composante Diffuse ---
    vec3 norm = normalize(v_worldNormal); // Normaliser ici est toujours une bonne pratique
    vec3 lightDir = normalize(u_lightPos - v_worldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_lightColor;
    
    // --- 3. Composante Spéculaire (Blinn-Phong) ---
    vec3 viewDir = normalize(u_viewPos - v_worldPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), u_shininess);
    vec3 specular = spec * u_lightColor; 

    // --- Résultat Final ---
    vec3 result = (ambient + diffuse + specular) * u_objectColor;
    fragColor = vec4(result, 1.0);
}