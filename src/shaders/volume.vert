R"zzz(
#version 330 core
uniform vec4 light_position;
uniform vec3 camera_position;
uniform float tex3d_layer;
in vec4 vertex_position;
in vec4 normal;
in vec2 uv;
out vec4 vs_light_direction;
out vec4 vs_normal;
out vec2 vs_uv;
out vec4 vs_camera_direction;
void main() {
	float scale = 50.0;
	gl_Position = vec4(scale * (vertex_position.x), 
					   scale * (vertex_position.y + tex3d_layer + 1.0), 
					   scale * (vertex_position.z), 1.0);
	vs_light_direction = light_position /*- gl_Position*/;
	// vs_camera_direction = vec4(camera_position, 1.0) - gl_Position;
	vs_normal = normal;
	vs_uv = uv;
}
)zzz"
