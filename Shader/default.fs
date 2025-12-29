#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 Tangent;

uniform sampler2D ourTexture;
uniform sampler2D normalMap;
uniform bool useNormalMap;
uniform float normalStrength;
uniform bool normalDebug;
uniform bool doubleSided;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;
uniform float alphaCutoff;
uniform vec3 spotPos;
uniform vec3 spotDir;
uniform vec3 spotColor;
uniform float spotIntensity;
uniform float spotInnerCutoff;
uniform float spotOuterCutoff;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float albedoIntensity;

void main()
{
    vec4 texColor = texture(ourTexture, TexCoord);
    if (texColor.a < alphaCutoff) {
        discard;
    }
    vec3 albedo = texColor.rgb * albedoIntensity;

    vec3 norm = normalize(Normal);
    if (useNormalMap) {
        vec3 tangent = normalize(Tangent);
        tangent = normalize(tangent - dot(tangent, norm) * norm);
        vec3 bitangent = cross(norm, tangent);
        mat3 tbn = mat3(tangent, bitangent, norm);
        vec3 normalSample = texture(normalMap, TexCoord).rgb;
        normalSample = normalSample * 2.0 - 1.0;
        normalSample.xy *= normalStrength;
        norm = normalize(tbn * normalSample);
        if (normalDebug) {
            vec3 debugColor = normalSample * 0.5 + 0.5;
            FragColor = vec4(debugColor, 1.0);
            return;
        }
    }
    if (doubleSided && !gl_FrontFacing) {
        norm = -norm;
    }
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

    float distance = length(viewPos - FragPos);
    float fogFactor = exp(-fogDensity * distance);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    vec3 finalColor = mix(fogColor, color, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
