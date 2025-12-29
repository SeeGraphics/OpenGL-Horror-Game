#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 3) in vec3 aInstancePos;

out vec2 TexCoord;
out vec3 FragPos;

uniform mat4 view;
uniform mat4 projection;

void main() {
    vec3 worldPos = aPos + aInstancePos;
    FragPos = worldPos;
    gl_Position = projection * view * vec4(worldPos, 1.0);
    TexCoord = aTexCoord;
}
