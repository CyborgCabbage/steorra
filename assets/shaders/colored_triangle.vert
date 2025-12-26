#version 450

layout (location = 0) out vec3 outColor;

//push constants block
layout( push_constant ) uniform constants
{
	mat4 mvp;
} PushConstants;

void main() 
{
	//const array of positions for the triangle
	const vec3 positions[3] = vec3[3](
		vec3(1.f,1.f, 0.0f),
		vec3(-1.f,1.f, 0.0f),
		vec3(0.f,-1.f, 0.0f)
	);

	//const array of colors for the triangle
	const vec3 colors[3] = vec3[3](
		vec3(1.0f, 0.0f, 0.0f), //red
		vec3(0.0f, 1.0f, 0.0f), //green
		vec3(00.f, 0.0f, 1.0f)  //blue
	);

	//output the position of each vertex
	gl_Position = PushConstants.mvp * vec4(positions[gl_VertexIndex], 1.0f);
	outColor = colors[gl_VertexIndex];
}
