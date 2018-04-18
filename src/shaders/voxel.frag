R"zzz(
#version 330 core

uniform sampler2D ter_height;
uniform float sky_time;

in vec2 uv_coords;
in vec4 face_normal;
in vec4 vertex_normal;
in vec4 light_direction;
in vec4 world_position;
out vec4 fragment_color;
void main() {
	vec3 color = vec3(0.5, 1.0, 0.5);
	vec3 ambient = vec3(0.05, 0.1, 0.05);

	float dot_nl = dot(normalize(light_direction), normalize(face_normal));
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	color = clamp(dot_nl * color, 0.0, 1.0);
	if(int(sky_time*2)%2 == 1)
		color *= 0.3;
	fragment_color = vec4(color, 1.0) + vec4(ambient, 0.0);
}
)zzz"
