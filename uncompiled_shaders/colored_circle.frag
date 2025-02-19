#version 450

//shader input
layout (location = 0) in vec3 inColor;

//output write
layout (location = 0) out vec4 outFragColor;


void main()
{
	const float radius = 0.25;
	if (length(gl_PointCoord - vec2(0.5)) > radius) {
		discard;
	}
	outFragColor = vec4(1, 1, 1, 1.0f);
}