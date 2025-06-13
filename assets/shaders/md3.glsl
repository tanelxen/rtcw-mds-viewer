#shader vertex
#version 410 core
layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 uMVP;

out vec2 uv;

void main()
{
    gl_Position = uMVP * position;
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
