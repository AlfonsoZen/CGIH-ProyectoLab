#version 330 core
out vec4 outColor;
  
in vec3 Color;
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
    outColor = texture(ourTexture, TexCoord);
    if(outColor.a < 0.1)
        discard;
}
