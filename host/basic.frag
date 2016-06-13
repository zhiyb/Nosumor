#version 330

uniform vec3 colour;

out vec4 fragColour;

void main(void)
{
	fragColour = vec4(colour, 1.0);
}
