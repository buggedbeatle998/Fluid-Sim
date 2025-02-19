#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;

struct Vertex {

	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer { 
	Vertex vertices[];
};

layout(std140, set = 0, binding = 1) readonly buffer Offsets {
	vec2 offs[];
} offsets;


//push constants block
layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	VertexBuffer vertexBuffer;
} PushConstants;


void main() 
{
	gl_PointSize = 50;
	//load vertex data from device adress
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	vec2 off = offsets.offs[gl_VertexIndex];
	//output data
	if (gl_VertexIndex >= 8)
	gl_Position = PushConstants.render_matrix * vec4(v.position.xy + off, v.position.z, 1.0f);
	else
	gl_Position = PushConstants.render_matrix * vec4(v.position.xyz, 1.0f);
	//float x_from_cent = (v.uv_x - 0.5f) * 2;
	//float y_from_cent = (v.uv_y - 0.5f) * 2;
	//if (x_from_cent + y_from_cent < 1.0f)
	outColor = v.color.xyz;
	//else
	//	outColor = vec3(0, 0, 0);
	outUV.x = v.uv_x;
	outUV.y = v.uv_y;
}