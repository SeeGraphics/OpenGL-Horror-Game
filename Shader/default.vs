#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec4 instanceRow0;
layout (location = 5) in vec4 instanceRow1;
layout (location = 6) in vec4 instanceRow2;
layout (location = 7) in vec4 instanceRow3;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec3 Tangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool useInstancing;

void main()
{
    mat4 modelMatrix = model;
    if (useInstancing) {
        modelMatrix = mat4(instanceRow0, instanceRow1, instanceRow2, instanceRow3);
    }
    vec4 worldPos = modelMatrix * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    Normal = normalize(normalMatrix * aNormal);
    Tangent = normalize(normalMatrix * aTangent);
    gl_Position = projection * view * worldPos;
    TexCoord = aTexCoord;
}
