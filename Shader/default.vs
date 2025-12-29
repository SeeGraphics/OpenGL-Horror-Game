#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec3 Tangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalize(normalMatrix * aNormal);
    Tangent = normalize(normalMatrix * aTangent);
    gl_Position = projection * view * worldPos;
    TexCoord = aTexCoord;
}
