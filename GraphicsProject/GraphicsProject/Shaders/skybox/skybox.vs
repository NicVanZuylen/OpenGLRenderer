#version 440 core

layout(location = 0) in vec3 pos;

layout (std140) uniform GlobalMatrices
{
    mat4 view;
    mat4 projection;
};

out vec3 texCoords;

void main() 
{
    texCoords = pos;

    mat4 viewCpy = view;

    viewCpy[3][0] = 0.0f;
    viewCpy[3][1] = 0.0f;
    viewCpy[3][2] = 0.0f;

    gl_Position = projection * viewCpy * vec4(pos, 1.0f);
}