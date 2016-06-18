#version 330

uniform int status;
uniform vec2 size;

out vec4 fragColour;

const vec3 colours[] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0),
	vec3(1.0, 1.0, 0.0),
	vec3(0.0, 1.0, 1.0),
	vec3(1.0, 0.0, 1.0),
	vec3(1.0, 1.0, 1.0),
	vec3(0.75, 0.75, 0.75)
);

void main(void)
{
	int bit = int(gl_FragCoord.y) * 8 / int(size.y) + 8;
	int field = 1 << bit;
	if ((status & field) != 0)
		fragColour = vec4(colours[bit & 7], 1.0);
	else {
		float a = gl_FragCoord.y / size.y;
		fragColour = vec4(mix(vec3(0.1, 0.1, 0.1), vec3(0.2, 0.2, 0.2), a), 1.0);
	}
}
