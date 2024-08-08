#version 330 core
out vec4 FragColor;

in vec3 ourColor; // Color from the vertex shader

void main()
{
    FragColor = vec4(ourColor, 1.0);
}
