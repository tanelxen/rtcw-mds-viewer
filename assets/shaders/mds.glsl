#shader vertex
#version 410 core
layout (location = 0) in vec4 packed0;
layout (location = 1) in vec4 packed1;
layout (location = 2) in vec4 packed2;
layout (location = 3) in vec3 normal;
layout (location = 4) in vec2 texCoord;

uniform mat4 uBoneTransforms[128];
uniform mat4 uMVP;

out vec2 uv;

void main()
{
    vec4 pos = vec4(0);

    vec4 offset0 = vec4(packed0.xyz, 1.0);
    float boneWeight0 = fract(packed0.w) * 2;
    int boneIndex0 = int(packed0.w);
    
    pos += offset0 * uBoneTransforms[boneIndex0] * boneWeight0;

    
    vec4 offset1 = vec4(packed1.xyz, 1.0);
    float boneWeight1 = fract(packed1.w) * 2;
    int boneIndex1 = int(packed1.w);
    
    pos += offset1 * uBoneTransforms[boneIndex1] * boneWeight1;


    vec4 offset2 = vec4(packed2.xyz, 1.0);
    float boneWeight2 = fract(packed2.w) * 2;
    int boneIndex2 = int(packed2.w);
    
    pos += offset2 * uBoneTransforms[boneIndex2] * boneWeight2;

    gl_Position = uMVP * pos;
    uv = texCoord;
}

#shader fragment
#version 410 core

in vec2 uv;

uniform sampler2D s_texture;

//final color
out vec4 FragColor;

void main()
{
    FragColor = texture(s_texture, uv);
}
