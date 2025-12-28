#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D ourTexture;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;
uniform vec3 spotPos;
uniform vec3 spotDir;
uniform vec3 spotColor;
uniform float spotIntensity;
uniform float spotInnerCutoff;
uniform float spotOuterCutoff;

void main()
{
    vec3 albedo = texture(ourTexture, TexCoord).rgb;

    vec3 norm = normalize(Normal);
    vec3 lightDirNorm = normalize(-lightDir);
    float diff = max(dot(norm, lightDirNorm), 0.0);

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirNorm, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    vec3 ambient = ambientStrength * lightColor;
    vec3 diffuse = diffuseStrength * diff * lightColor;
    vec3 specular = specularStrength * spec * lightColor;

    vec3 spotDirNorm = normalize(spotDir);
    vec3 toFrag = normalize(FragPos - spotPos);
    float theta = dot(toFrag, spotDirNorm);
    float epsilon = spotInnerCutoff - spotOuterCutoff;
    float spotFactor = clamp((theta - spotOuterCutoff) / epsilon, 0.0, 1.0);

    vec3 spotLightDir = normalize(spotPos - FragPos);
    float spotDiff = max(dot(norm, spotLightDir), 0.0);
    vec3 spotReflect = reflect(-spotLightDir, norm);
    float spotSpec = pow(max(dot(viewDir, spotReflect), 0.0), shininess);

    vec3 spotDiffuse =
        diffuseStrength * spotDiff * spotColor * spotIntensity * spotFactor;
    vec3 spotSpecular =
        specularStrength * spotSpec * spotColor * spotIntensity * spotFactor;

    vec3 color =
        (ambient + diffuse + spotDiffuse) * albedo + specular + spotSpecular;
    FragColor = vec4(color, 1.0);
}
