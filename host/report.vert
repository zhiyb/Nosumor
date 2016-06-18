#version 330

uniform mat4 mvpMatrix;

in vec2 position;

void main(void)
{
	gl_Position = mvpMatrix * vec4(position, 0.0, 1.0);
}
