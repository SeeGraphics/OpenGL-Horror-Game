#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 FragPos;

uniform sampler2D grassTexture;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float ambientStrength;
uniform float diffuseStrength;
uniform vec3 spotPos;
uniform vec3 spotDir;
uniform vec3 spotColor;
uniform float spotIntensity;
uniform float spotInnerCutoff;
uniform float spotOuterCutoff;
uniform float grassIntensity;
uniform vec3 fogColor;
uniform float fogDensity;

void main() {
    vec4 texColor = texture(grassTexture, TexCoord);
    if (texColor.a < 0.6) {
        discard;
    }

    vec3 normal = vec3(0.0, 1.0, 0.0);
    vec3 lightDirNorm = normalize(-lightDir);
    float diff = max(dot(normal, lightDirNorm), 0.0);

    vec3 ambient = ambientStrength * lightColor;
    vec3 diffuse = diffuseStrength * diff * lightColor;

    vec3 spotDirNorm = normalize(spotDir);
    vec3 toFrag = normalize(FragPos - spotPos);
    float theta = dot(toFrag, spotDirNorm);
    float epsilon = spotInnerCutoff - spotOuterCutoff;
    float spotFactor = clamp((theta - spotOuterCutoff) / epsilon, 0.0, 1.0);

    vec3 spotLightDir = normalize(spotPos - FragPos);
    float spotDiff = max(dot(normal, spotLightDir), 0.0);
    vec3 spotDiffuse =
        diffuseStrength * spotDiff * spotColor * spotIntensity * spotFactor;

    vec3 color = (ambient + diffuse + spotDiffuse) * texColor.rgb;
    color *= grassIntensity;

    float distance = length(viewPos - FragPos);
    float fogFactor = exp(-fogDensity * distance);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    vec3 finalColor = mix(fogColor, color, fogFactor);

    FragColor = vec4(finalColor, texColor.a);
}
