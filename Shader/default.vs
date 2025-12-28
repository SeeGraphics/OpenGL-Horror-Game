#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in mat4 instancedMatrix;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    mat4 world = model * instancedMatrix;
    vec4 worldPos = world * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    Normal = normalize(transpose(inverse(mat3(world))) * aNormal);
    gl_Position = projection * view * worldPos;
    TexCoord = aTexCoord;
}
