#version 440 core

uniform samplerCube cubemap;

layout (location = 0) out vec4 fragDiffuseOut;
layout (location = 1) out vec4 fragPositionOut;
layout (location = 2) out vec4 fragNormalOut;
layout (location = 3) out vec4 fragSpecularOut;
layout (location = 4) out vec3 fragRoughnessOut;
layout (location = 5) out vec3 fragBrightOut;

in vec3 texCoords;

#define SKYBOX_DEPTH 0.9999999f

void main() 
{
    fragDiffuseOut = texture(cubemap, texCoords);

    fragBrightOut = vec3(0.0f);

    gl_FragDepth = SKYBOX_DEPTH;
}