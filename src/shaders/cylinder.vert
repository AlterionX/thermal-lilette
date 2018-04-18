R"zzz(#version 330 core
uniform mat4 bone_transform; // transform the cylinder to the correct configuration
const float kPi = 3.1415926535897932384626433832795;
uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;
in vec4 vertex_position;


// Note: you need call sin/cos to transform the input mesh to a cylinder

void main() {
	vec4 cyl_pos = vec4(sin(2 * kPi * vertex_position.x),
						vertex_position.y,
						cos(2 * kPi * vertex_position.x),
						1.0);

	mat4 mvp = projection * view * model * bone_transform;
	gl_Position = mvp * cyl_pos;
}

)zzz"
