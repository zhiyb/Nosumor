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
	int y = int(gl_FragCoord.y);
	int bit = y * 8 / int(size.y) + 8;
	int bitP = (y - 1) * 8 / int(size.y) + 8;
	int bitN = (y + 1) * 8 / int(size.y) + 8;
	int field = 1 << bit;
	if (bitP != bit || bitN != bit || int(gl_FragCoord.x) % 20 == 0)
		fragColour = vec4(0.3, 0.3, 0.3, 1.0);
	else if ((status & field) != 0)
		fragColour = vec4(colours[bit & 7], 1.0);
	else
		discard;
}
